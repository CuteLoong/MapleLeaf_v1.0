#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set=0, binding = 1) uniform sampler2D Lighting;

void main()
{
    vec2 uv = vec2(inUV.x, 1.0 - inUV.y);
    vec4 color = texture(Lighting, uv);
    outColor = color;
}