#ifndef RAY_TRACING_COMMON_GLSL
#define RAY_TRACING_COMMON_GLSL

struct HitPayLoad
{
    vec3 radiance;
    int depth;
    vec3 accBrdf;
    int done;
    vec4 nextOrigin;
    vec4 nextDir;
    uint randomSeed;
    mat4 covariance;
};

#endif // RAY_TRACING_COMMON_GLSL