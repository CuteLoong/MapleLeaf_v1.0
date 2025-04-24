#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

layout (set = 0, binding = 1) uniform UniformSkybox 
{
	mat4 projection;
	mat4 view;
    int index;
} uniformSkybox;

// layout(push_constant) uniform PushObject {
//     mat4 projection;
// 	mat4 view;
// } pushObject;

layout (location = 0) out vec3 outUVW;

const vec3 positions[36] = vec3[](
    vec3(-1.0f, -1.0f, -1.0f),  
    vec3( 1.0f, -1.0f, -1.0f),  
    vec3( 1.0f,  1.0f, -1.0f),  
    vec3( 1.0f,  1.0f, -1.0f),  
    vec3(-1.0f,  1.0f, -1.0f),  
    vec3(-1.0f, -1.0f, -1.0f),  

    vec3(-1.0f, -1.0f,  1.0f),  
    vec3( 1.0f, -1.0f,  1.0f),  
    vec3( 1.0f,  1.0f,  1.0f),  
    vec3( 1.0f,  1.0f,  1.0f),  
    vec3(-1.0f,  1.0f,  1.0f),  
    vec3(-1.0f, -1.0f,  1.0f),  

    vec3(-1.0f,  1.0f,  1.0f),  
    vec3(-1.0f,  1.0f, -1.0f),  
    vec3(-1.0f, -1.0f, -1.0f),  
    vec3(-1.0f, -1.0f, -1.0f),  
    vec3(-1.0f, -1.0f,  1.0f),  
    vec3(-1.0f,  1.0f,  1.0f),  

    vec3( 1.0f,  1.0f,  1.0f),  
    vec3( 1.0f,  1.0f, -1.0f),  
    vec3( 1.0f, -1.0f, -1.0f),  
    vec3( 1.0f, -1.0f, -1.0f),  
    vec3( 1.0f, -1.0f,  1.0f),  
    vec3( 1.0f,  1.0f,  1.0f),  

    vec3(-1.0f, -1.0f, -1.0f),  
    vec3( 1.0f, -1.0f, -1.0f),  
    vec3( 1.0f, -1.0f,  1.0f),  
    vec3( 1.0f, -1.0f,  1.0f),  
    vec3(-1.0f, -1.0f,  1.0f),  
    vec3(-1.0f, -1.0f, -1.0f),  

    vec3(-1.0f,  1.0f, -1.0f),  
    vec3( 1.0f,  1.0f, -1.0f),  
    vec3( 1.0f,  1.0f,  1.0f),  
    vec3( 1.0f,  1.0f,  1.0f),  
    vec3(-1.0f,  1.0f,  1.0f),  
    vec3(-1.0f,  1.0f, -1.0f)  
    );

void main() 
{
	vec4 worldPosition = vec4(positions[gl_VertexIndex], 1.0f);

	outUVW = positions[gl_VertexIndex];
	gl_Position = uniformSkybox.projection * uniformSkybox.view * worldPosition;
}