#ifndef SHADING_FRAME_GLSL
#define SHADING_FRAME_GLSL

#include <Utils/MathHelpers.glsl>

struct Frame
{
    vec3 T; // Shading tangent. Normalized
    vec3 B; // Shading bitangent. Normalized
    vec3 N; // Shading normal. Normalized
};



void initFrame(inout Frame frame, vec3 normalW, vec4 tangentW)
{
    frame.N = normalize(normalW);
    
    float NDotT = dot(frame.N, tangentW.xyz);
    bool nonParallel = abs(NDotT) < 0.999f;
    bool nonZero = dot(tangentW.xyz, tangentW.xyz) > 0.0001f;
    bool valid = abs(tangentW.w) == 1.0f && nonZero && nonParallel;

    if(valid) {
        frame.T = normalize(tangentW.xyz - NDotT * frame.N);
        frame.B = cross(frame.N, frame.T) * tangentW.w;
    }
    else {
        frame.T = perp_stark(frame.N);
        frame.B = cross(frame.N, frame.T);
    }
}

vec3 toWorld(in Frame frame, vec3 v)
{
    return v.x * frame.T + v.y * frame.B + v.z * frame.N;
}

vec3 toLocal(in Frame frame, vec3 v)
{
    return vec3(dot(v, frame.T), dot(v, frame.B), dot(v, frame.N));
}

#endif