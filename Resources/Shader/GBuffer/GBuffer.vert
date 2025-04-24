#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include <Misc/Camera.glsl>
#include <Misc/Parameters.glsl>

layout(set = 0, binding = 1) readonly buffer InstanceDatas
{
    GPUInstanceData instanceData[];
} instanceDatas;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out flat uint outMaterialId;
layout(location = 5) out flat uint outInstanceID;
layout(location = 6) out flat uint outIsAreaLight;
layout(location = 7) out vec4 hPos;
layout(location = 8) out vec4 prevHPos;

void main()
{
    uint instanceIndex = gl_InstanceIndex;
    GPUInstanceData instanceData = instanceDatas.instanceData[nonuniformEXT(instanceIndex)];
    uint materialId = instanceData.materialID;

    vec4 position = vec4(inPosition, 1.0f);
    vec4 normal = vec4(inNormal, 0.0f);
    vec4 tangent = vec4(inTangent, 0.0f);

    vec4 worldPosition = instanceData.modelMatrix * position;
    mat3 normalMatrix = transpose(inverse(mat3(instanceData.modelMatrix)));

    mat4 projection = GetProjection();
    mat4 ortho = camera.ortho;
    mat4 view = GetView();

    gl_Position = projection * view * worldPosition;
    // gl_Position.z = gl_Position.z * 0.5 + 0.5;

    outPosition = worldPosition.xyz;
    outUV = inUV;
	outNormal = normalMatrix * normalize(normal.xyz);
    outTangent = normalMatrix * normalize(tangent.xyz);
    outMaterialId = materialId;
    outInstanceID = instanceIndex;
    outIsAreaLight = instanceData.isAreaLight;
    hPos = projection * view * worldPosition;
    prevHPos = camera.prevProjection * camera.prevView * instanceData.prevModelMatrix * position;
}