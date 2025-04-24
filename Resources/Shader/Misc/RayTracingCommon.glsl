#ifndef RAY_TRACING_COMMON_GLSL
#define RAY_TRACING_COMMON_GLSL

struct HitPayLoad
{
    vec3 diffuseRadiance;
    int depth;
    vec3 specularRadiance;
    int done;
    vec3 accDiffuseBRDF;
    uint randomSeed;
    vec3 accSpecularBRDF;
    vec4 nextOrigin;
    vec4 nextDir;
};

#endif // RAY_TRACING_COMMON_GLSL