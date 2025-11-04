#ifndef SPLIT_MIX_64_GLSL
#define SPLIT_MIX_64_GLSL

#extension GL_ARB_gpu_shader_int64 : require

uint64_t asuint64(uint lowbits, uint highbits)
{
    return (uint64_t(highbits) << 32) | uint64_t(lowbits);
}

// 64-bit split-mix pseudorandom number generator.
uint64_t splitMix64(inout uint64_t seed)
{
    seed += 0x9e3779b97f4a7c15ul;
    uint64_t z = seed;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ul;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebul;
    return z ^ (z >> 31);
}

uint nextRandom(inout uint64_t seed)
{
    return uint(splitMix64(seed));
}

uint64_t createSplitMix64(uint s0, uint s1)
{
    return asuint64(s0, s1);
}

#endif // SPLIT_MIX_64_GLSL