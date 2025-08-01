#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include <Misc/RayTracingCommon.glsl>
#include <Misc/Constants.glsl>
#include <Misc/Parameters.glsl>
#include <Materials/Fresnel.glsl>
#include <Materials/BRDF.glsl>
#include <Sampling/TinyEncryptionSample.glsl>
#include <Lighting/Lighting.glsl>
#include <Lighting/LTC.glsl>

layout(location = 0) rayPayloadInEXT HitPayLoad prd;
hitAttributeEXT vec3 attribs;

layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v[]; };
layout(buffer_reference, scalar) readonly buffer Indices {uint i[]; };

layout(set = 0, binding = 4, scalar) uniform UniformGeometry {
	uint64_t vertexAddress;
	uint64_t indexAddress; 
} uniformGeometry;

layout(set=0, binding = 5) uniform UniformScene {
	mat4 shadowMatrix;

	int pointLightsCount;
	int directionalLightsCount;
	int areaLightsCount;
	int skyboxLoaded;
	float wallroughness;
} uniformScene;

layout(set = 0, binding = 6) readonly buffer InstanceDatas
{
    GPUInstanceData instanceData[];
} instanceDatas;

layout(set = 0, binding = 7) buffer MaterialDatas
{
    GPUMaterialData materialData[];
} materialDatas;

layout(set=0, binding = 8) buffer BufferPointLights {
	PointLight lights[];
} bufferPointLights;

layout(set=0, binding = 9) buffer BufferDirectionalLights {
	DirectionalLight lights[];
} bufferDirectionalLights;

layout(set=0, binding = 10) buffer BufferAreaLights {
	AreaLight lights[];
} bufferAreaLights;


layout(set=0, binding = 11) uniform sampler2D samplerBRDF;
layout(set=0, binding = 12) uniform samplerCube samplerIrradiance;
layout(set=0, binding = 13) uniform samplerCube samplerPrefiltered;

layout(set=0, binding = 14) uniform sampler2D LTC1;
layout(set=0, binding = 15) uniform sampler2D LTC2;

layout(set = 1, binding = 0) uniform sampler2D ImageSamplers[];

void generateBasis(vec3 N, out vec3 up, out vec3 right, out vec3 forward)
{
    up = abs(N.z) < 0.999f ? vec3(0, 0, 1) : vec3(1, 0, 0);
    right = normalize(cross(up, N));
    forward = cross(N, right);
}

vec3 localToWorld(vec3 localVector, vec3 N)
{
	vec3 up, right, forward;
	generateBasis(N, up, right, forward);

	return localVector.x * right + localVector.y * forward + localVector.z * N;
}

void main()
{
	GPUInstanceData instanceData = instanceDatas.instanceData[gl_InstanceCustomIndexEXT];
	GPUMaterialData material = materialDatas.materialData[instanceData.materialID];
	Vertices vertices = Vertices(uniformGeometry.vertexAddress);
	Indices indices = Indices(uniformGeometry.indexAddress);

	uint indexOffset = instanceData.indexOffset;
	uint vertexOffset = instanceData.vertexOffset;

	vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
	ivec3 index = ivec3(indices.i[indexOffset + gl_PrimitiveID * 3], indices.i[indexOffset + gl_PrimitiveID * 3 + 1], indices.i[indexOffset + gl_PrimitiveID * 3 + 2]);
	Vertex v0 = vertices.v[vertexOffset + index.x];
	Vertex v1 = vertices.v[vertexOffset + index.y];
	Vertex v2 = vertices.v[vertexOffset + index.z];

	vec3 position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
	vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
	vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

	vec3 worldPosition = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0f));
	vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

	vec4 baseColor = material.baseColor;
	float metallic = material.metallic;
	float roughness = material.roughness;
	// if(prd.depth == 1 && gl_InstanceCustomIndexEXT != 5) return;
	if(gl_InstanceCustomIndexEXT != 5) roughness = uniformScene.wallroughness;

	int baseColorTex = material.baseColorTex;
	int normalTex = material.normalTex;
	int materialTex = material.materialTex;

	if(baseColorTex != -1) baseColor = texture(ImageSamplers[baseColorTex], texCoord);
	if(materialTex != -1) {
		vec4 textureMaterial = texture(ImageSamplers[materialTex], texCoord);
		metallic *= textureMaterial.r;
		roughness *= textureMaterial.g;
	}
	if(normalTex != -1) {
		vec3 tangentNormal = texture(ImageSamplers[normalTex], texCoord).rgb * 2.0f - 1.0f;

		vec3 tangent = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
		tangent = normalize(vec3(tangent * gl_WorldToObjectEXT));

		vec3 N = normalize(worldNormal);
		vec3 T = normalize(tangent);
		T = normalize(T - dot(T, N) * N);
		vec3 B = -normalize(cross(N, T));
		mat3 TBN = mat3(T, B, N);

		worldNormal = normalize(TBN * tangentNormal);
	}

	vec3 Lo = vec3(0.0f);

	vec3 V = -normalize(gl_WorldRayDirectionEXT);
	vec3 F0 = vec3(0.04f);
	vec3 diffuseColor = baseColor.rgb * (1.0f - F0) * (1.0f - metallic);
	vec3 specularColor = mix(F0, baseColor.rgb, metallic);

	if(normal != vec3(0.0f) && instanceData.isAreaLight == 0.0f)
	{
		vec3 N = worldNormal;
		vec3 R = reflect(-V, N);
		float NdotV = clamp(dot(N, V), 0.0f, 1.0f);

		for(int i = 1; i <= uniformScene.pointLightsCount; i++) 
		{
			PointLight light = bufferPointLights.lights[i];
			vec3 L = normalize(light.position - worldPosition);
			float d = length(L);
			L = normalize(L);
			float NoL = clamp(dot(worldNormal, L), 0.0f, 1.0f);

			vec3 radiance = calcAttenuation(d, light.attenuation) * light.color.rgb;

			vec3 brdf = prd.depth >= 1 ? DiffuseReflectionDisneyEvalWeight(diffuseColor, roughness, N, L, V) : DiffuseReflectionDisneyEvalWeight(diffuseColor, roughness, N, L, V) + SpecularReflectionMicrofacetEvalWeight(specularColor, roughness, N, L, V);

			// Lo += brdf * radiance;
		}

		for(int i = 1; i <= uniformScene.directionalLightsCount; i++) {
			DirectionalLight light = bufferDirectionalLights.lights[i];
			vec3 L = normalize(-light.direction);

			float NoL = clamp(dot(worldNormal, L), 0.0f, 1.0f);
			vec3 radiance = light.color.rgb;

			vec3 brdf = prd.depth >= 1 ? DiffuseReflectionDisneyEvalWeight(diffuseColor, roughness, N, L, V) : DiffuseReflectionDisneyEvalWeight(diffuseColor, roughness, N, L, V) + SpecularReflectionMicrofacetEvalWeight(specularColor, roughness, N, L, V);

			// Lo += brdf * radiance;
		}

		//AreaLight: use roughness and sqrt(1-cos_theta) to sample M_texture
    	vec2 ltcUV = LTC_Coords(NdotV, roughness);
		
		vec4 t2 = texture(LTC2, ltcUV); // Get 2 parameters for Fresnel calculation

		mat3 Minv = LTC_Matrix(LTC1, ltcUV);
		// iterate through all area lights
		for (int i = 1; i <= uniformScene.areaLightsCount; i++)
		{
			AreaLight areaLight = bufferAreaLights.lights[i];
			vec3 points[4] = {areaLight.points[0].xyz, areaLight.points[1].xyz, areaLight.points[2].xyz, areaLight.points[3].xyz};

			bool isTwoSide = true;
			vec3 diffuseLTC = LTC_Evaluate(N, V, worldPosition, mat3(1), points, isTwoSide);
			vec3 specularLTC = LTC_Evaluate(N, V, worldPosition, Minv, points, isTwoSide);

			specularLTC *= specularColor * t2.x + (1.0f - specularColor) * t2.y;
			vec4 lightColor = areaLight.color;

			// Add contribution
			Lo += prd.depth >= 1 ? lightColor.rgb * areaLight.intensity * (diffuseColor * diffuseLTC) : lightColor.rgb * areaLight.intensity * (specularLTC + diffuseColor * diffuseLTC);
			// Lo += lightColor.rgb * areaLight.intensity * (specularLTC + diffuseColor * diffuseLTC);

		}

		if(uniformScene.skyboxLoaded == 1) {
			vec3 brdfPreIntegrated = texture(samplerBRDF, vec2(NdotV, roughness * roughness)).rgb;
			vec3 reflection = prefilteredReflection(R, roughness, samplerPrefiltered).rgb;	
			vec3 specular = reflection * (specularColor * brdfPreIntegrated.r + brdfPreIntegrated.g);

			vec3 irradiance = texture(samplerIrradiance, N).rgb;
			vec3 diffuseLo = irradiance * diffuseColor * brdfPreIntegrated.b;

			vec3 ambient = (diffuseLo + specular) * 0.2f;

			Lo += diffuseLo;
		}
	}
	else {
		Lo = baseColor.rgb;
		prd.diffuseRadiance = Lo;
		return;
	}
	
	vec2 Xi = vec2(TinyEncryptionRandom(prd.randomSeed), TinyEncryptionRandom(prd.randomSeed));

	float pdf = 1.0f;
    vec3 H = localToWorld(sampleGGX_NDF(roughness*roughness, Xi, pdf), worldNormal);
	float VoH = clamp(dot(V, H), 0.0f, 1.0f);
	pdf = pdf / (4.0f * VoH);

	prd.nextOrigin = vec4(worldPosition, 1.0f);
	prd.nextDir = vec4(normalize(reflect(-V, H)), 0.0f);

	prd.diffuseRadiance = Lo * prd.accDiffuseBRDF * 0.5f;
	prd.specularRadiance = Lo * prd.accSpecularBRDF * 0.5f;

	prd.accDiffuseBRDF *= DiffuseReflectionDisneyEval(diffuseColor, roughness, worldNormal, prd.nextDir.xyz, V) / pdf; // maybe a problity to select diffuse or specular
	prd.accSpecularBRDF *= SpecularReflectionMicrofacetEvalWeight(specularColor, roughness, worldNormal, prd.nextDir.xyz, V);

	prd.done = 0;
}
