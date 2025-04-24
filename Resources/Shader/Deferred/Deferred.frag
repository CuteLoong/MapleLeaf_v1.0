#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#include <Misc/Camera.glsl>

layout(set=0, binding = 1) uniform UniformScene {
	mat4 shadowMatrix;

	int pointLightsCount;
	int directionalLightsCount;
	int areaLightsCount;
	int skyboxLoaded;
} uniformScene;

struct PointLight {
	vec4 color;
	vec3 position;
	float pad;
	vec3 attenuation;
	float pad1;
};

struct DirectionalLight {
	vec4 color;
	vec3 direction;
	float pad;
};

struct AreaLight
{
	vec4 color;
	vec4 points[4];
    uint twoSide;
    float intensity;
};

layout(set=0, binding = 2) buffer BufferPointLights {
	PointLight lights[];
} bufferPointLights;

layout(set=0, binding = 3) buffer BufferDirectionalLights {
	DirectionalLight lights[];
} bufferDirectionalLights;

layout(set=0, binding = 4) uniform sampler2D inPosition;
layout(set=0, binding = 5) uniform sampler2D inDiffuse;
layout(set=0, binding = 6) uniform sampler2D inNormal;
layout(set=0, binding = 7) uniform sampler2D inMaterial;
//layout(set=0, binding = 8) uniform sampler2D inShadowMap;
// layout(set=0, binding = 9) uniform sampler2D inAOMap;

layout(set=0, binding = 10) uniform sampler2D samplerBRDF;
layout(set=0, binding = 11) uniform samplerCube samplerIrradiance;
layout(set=0, binding = 12) uniform samplerCube samplerPrefiltered;

layout(set=0, binding = 13) buffer BufferAreaLights {
	AreaLight lights[];
} bufferAreaLights;

layout(set=0, binding = 14) uniform sampler2D LTC1; // for inverse M
layout(set=0, binding = 15) uniform sampler2D LTC2; // GGX norm, fresnel, 0(unused), sphere

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColour;

#include <Lighting/Lighting.glsl>
#include <Misc/Constants.glsl>
#include <Materials/Fresnel.glsl>
#include <Materials/BRDF.glsl>
#include <Lighting/LTC.glsl>

void main() {
	vec2 uv = vec2(inUV.x, 1.0f - inUV.y);

	vec3 worldPosition = texture(inPosition, uv).rgb;
	vec4 shadowCoords = uniformScene.shadowMatrix * vec4(worldPosition, 1.0f);

	vec3 baseColor = texture(inDiffuse, uv).rgb;
	vec3 normal = texture(inNormal, uv).rgb;
	vec3 material = texture(inMaterial, uv).rgb;
	// vec3 ao = texture(inAOMap, uv).rgb;

	float metallic = material.r;
	float roughness = material.g;
	float isAreaLight = material.b;
	
	vec3 Lo = vec3(0.0f);

	if(normal != vec3(0.0f) && isAreaLight != 1.0f) {
		vec3 N = normalize(normal);
		vec3 V = normalize(camera.cameraPosition.xyz - worldPosition);
		vec3 R = reflect(-V, N);

		float NdotV = clamp(dot(N, V), 0.0f, 1.0f);

		// FOR Ficus back face
		// if (NdotV == 0.0f) N = -N;
		// NdotV = clamp(dot(N, V), 0.0f, 1.0f);

		vec3 F0 = vec3(0.04f);
		vec3 diffuseColor = baseColor.rgb * (1.0f - F0) * (1.0f - metallic);
		vec3 specularColor = mix(F0, baseColor, metallic);

		for(int i = 1; i <= uniformScene.pointLightsCount; i++)
		{
			PointLight light = bufferPointLights.lights[i];
			vec3 L = light.position - worldPosition;
			float d = length(L);
			L = normalize(L);
			float NoL = clamp(dot(N, L), 0.0f, 1.0f);

			vec3 radiance = calcAttenuation(d, light.attenuation) * light.color.rgb;

			vec3 brdf = DiffuseReflectionDisneyEvalWeight(diffuseColor, roughness, N, L, V) + SpecularReflectionMicrofacetEvalWeight(specularColor, roughness, N, L, V);

			Lo += brdf * radiance * NoL;
		}

		for(int i = 1; i <= uniformScene.directionalLightsCount; i++)
		{
			DirectionalLight light = bufferDirectionalLights.lights[i];
			vec3 L = normalize(-light.direction);

			float NoL = clamp(dot(N, L), 0.0f, 1.0f);
			vec3 radiance = light.color.rgb;

			vec3 brdf = DiffuseReflectionDisneyEvalWeight(diffuseColor, roughness, N, L, V) + SpecularReflectionMicrofacetEvalWeight(specularColor, roughness, N, L, V);

			//float shadowValue = shadowFactor(shadowCoords, inShadowMap);

			//Lo += brdf * radiance * shadowValue;
			Lo += brdf * radiance * NoL;
		}

		//AreaLight: use roughness and sqrt(1-cos_theta) to sample M_texture
    	vec2 ltcUV = LTC_Coords(NdotV, roughness);
		
		vec4 t2 = texture(LTC2, ltcUV); // Get 2 parameters for Fresnel calculation

		mat3 Minv = LTC_Matrix(LTC1, ltcUV);
		// iterate through all area lights
		for (int i = 1; i <= uniformScene.areaLightsCount && isAreaLight == 0.0f; i++)
		{
			AreaLight areaLight = bufferAreaLights.lights[i];
			vec3 points[4] = {
				areaLight.points[0].xyz,
				areaLight.points[1].xyz,
				areaLight.points[2].xyz,
				areaLight.points[3].xyz
			};

			bool isTwoSide = true;
			vec3 diffuseLTC = LTC_Evaluate(N, V, worldPosition, mat3(1), points, isTwoSide);
			vec3 specularLTC = LTC_Evaluate(N, V, worldPosition, Minv, points, isTwoSide);

			specularLTC *= specularColor * t2.x + (1.0f - specularColor) * t2.y;
			vec4 lightColor = areaLight.color;

			// Add contribution
			Lo += lightColor.rgb * areaLight.intensity * (specularLTC + diffuseColor * diffuseLTC);
		}			

		if(uniformScene.skyboxLoaded == 1)
		{
			vec3 brdfPreIntegrated = texture(samplerBRDF, vec2(NdotV, roughness * roughness)).rgb;
			vec3 reflection = prefilteredReflection(R, roughness, samplerPrefiltered).rgb;	
			vec3 specular = reflection * (specularColor * brdfPreIntegrated.r + brdfPreIntegrated.g);

			vec3 irradiance = texture(samplerIrradiance, N).rgb;
			vec3 diffuseLo = irradiance * diffuseColor * brdfPreIntegrated.b;

			vec3 ambient = (diffuseLo + specular); //  (diffuseLo * 2.5f + specular * 0.2f
		// // vec3 ambient = (diffuseLo + specular) * 2.0f ;
		// vec3 ambient = (specular) * 1.0f ;

			Lo += ambient;
		}
		
	}
	else {
		Lo = baseColor;
		if(isAreaLight == 1.0f) Lo *= 10.0f;
	}
	
	outColour = vec4(Lo, 1.0f);
}