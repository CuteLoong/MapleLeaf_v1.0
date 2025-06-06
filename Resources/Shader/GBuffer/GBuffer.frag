#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_GOOGLE_include_directive : require

#include <Misc/Camera.glsl>

struct GPUMaterialData
{
    vec4  baseColor;
    float metallic;
    float roughness;
    int   baseColorTex;
    int   normalTex;
    int   materialTex;
};

layout(set = 0, binding = 2) buffer MaterialDatas
{
    GPUMaterialData materialData[];
} materialDatas;

layout(set = 1, binding = 0) uniform sampler2D ImageSamplers[];

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in flat uint inMaterialId;
layout(location = 5) in flat uint inInstanceID;
layout(location = 6) in flat uint inIsAreaLight;
layout(location = 7) in vec4 hPos;
layout(location = 8) in vec4 prevHPos;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outDiffuse;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outMaterial;
layout(location = 4) out vec4 outMotionVetcor;
layout(location = 5) out float outInstanceID;

void main() 
{	
	GPUMaterialData materialData = materialDatas.materialData[inMaterialId];

	vec4 diffuse = materialData.baseColor;
	vec3 normal = normalize(inNormal);
	vec3 material = vec3(materialData.metallic, materialData.roughness, float(inIsAreaLight));

	int baseColorTex = materialData.baseColorTex;
	int normalTex = materialData.normalTex;
	int materialTex = materialData.materialTex;

	if(baseColorTex != -1) diffuse = texture(ImageSamplers[baseColorTex], inUV);

	if(materialTex != -1) {
		vec4 textureMaterial = texture(ImageSamplers[materialTex], inUV);
		material.x *= textureMaterial.r;
		material.y *= textureMaterial.g;
	}

	if(normalTex != -1) {
		vec3 tangentNormal = texture(ImageSamplers[normalTex], inUV).rgb * 2.0f - 1.0f;

		vec3 N = normal;
		vec3 T = normalize(inTangent);
		T = normalize(T - N * dot(T, N));
		vec3 B = -normalize(cross(N, T));

		mat3 TBN = mat3(T, B, N);
		normal = normalize(TBN * tangentNormal);
	}

	vec3 vPos = (hPos.xyz / hPos.w + 1.0f) * 0.5f;
	vec3 prevVPos = (prevHPos.xyz / prevHPos.w + 1.0f) * 0.5f;

	vec2 curPixelUV = ivec2(vPos.xy * camera.pixelSize.xy) * camera.pixelSize.zw; 
    vec2 prevPixelUV = ivec2(prevVPos.xy * camera.pixelSize.xy) * camera.pixelSize.zw;

	vec2 mv = curPixelUV - prevPixelUV;

	mv.y = -mv.y;
	outMotionVetcor = vec4(mv, 0.0f, 1.0f);

	outPosition = vec4(inPosition, 1.0f);
	outDiffuse = diffuse;
	outNormal = vec4(normalize(normal), 1.0f);
	outMaterial = vec4(material, 1.0f);

	outInstanceID = float(inInstanceID);
}