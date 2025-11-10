#ifndef EMISSIVE_UNIFORM_SAMPLER_GLSL
#define EMISSIVE_UNIFORM_SAMPLER_GLSL

struct TriangleLightSample
{
    uint  triangleIndex;      ///< Index of the sampled triangle.
    vec3  posW;               ///< Sampled point on the light source in world space.
    vec3  normalW;            ///< Normal of the sampled point on the light source in world space.
    vec3  dir;                ///< Normalized direction from the shading point to the sampled point on the light source in world space.
    float dist;               ///< Distance from the shading point to the sampled point.
    vec3  Le;                 ///< Emitted radiance. This is zero if the light is back-facing or sample is invalid.
    float pdf;                ///< Probability density with respect to solid angle from the shading point. The range is [0,inf] (inclusive), where pdf == 0.0 indicated an invalid sample.
    vec2  uv;                 ///< Light sample barycentric coords over the triangle
};



#endif // EMISSIVE_UNIFORM_SAMPLER_GLSL