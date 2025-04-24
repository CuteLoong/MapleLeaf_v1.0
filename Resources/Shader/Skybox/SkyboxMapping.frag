#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform UniformSkybox 
{
	mat4 projection;
	mat4 view;
    int index;
} uniformSkybox;

layout (set = 0, binding = 2) uniform UniformParams {
	float exposure;
	float gamma;
} uniformParams;

layout (set = 0, binding = 3) uniform sampler2D EquirectangularMap;
layout (set = 0, binding = 4) writeonly uniform imageCube CubeMap;

// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

//blender color space: sRGB
vec3 sRGBtoLinear(vec3 srgbIn)
{
    vec3 linOut = pow(srgbIn, vec3(2.2));
    return linOut;
}

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main() 
{
	vec2 uv = SampleSphericalMap(normalize(inUVW));
	vec3 color = texture(EquirectangularMap, uv).rgb;
	color = Uncharted2Tonemap(color * uniformParams.exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	

	color = pow(color, vec3(1.0f / uniformParams.gamma));

	outColor = vec4(color, 1.0);
	imageStore(CubeMap, ivec3(gl_FragCoord.xy, uniformSkybox.index), vec4(color, 1.0));
}