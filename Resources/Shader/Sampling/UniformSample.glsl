#ifndef UNIFORM_SAMPLE_GLSL
#define UNIFORM_SAMPLE_GLSL

#include "Pseudorandom/SplitMix64.glsl"
#include "Pseudorandom/Xoshiro.glsl"
#include <Math/BitTricks.glsl>

uvec4 UniformSampleInit(uvec2 pixel, uint sampleNumber)
{
    uint64_t rng = createSplitMix64(interleave_32bit(pixel), sampleNumber);
    uint64_t s0 = splitMix64(rng);
    uint64_t s1 = splitMix64(rng);

    uvec4 seed = uvec4(uint(s0), uint(s0 >> 32), uint(s1), uint(s1 >> 32));

    return createXoshiro(seed);
}

float UniformSampleNext(inout uvec4 state)
{
    return (nextRandom(state) >> 8) / float(0x01000000);
}

#endif // UNIFORM_SAMPLE_GLSL