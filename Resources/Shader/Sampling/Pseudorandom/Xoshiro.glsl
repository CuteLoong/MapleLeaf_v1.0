#ifndef XOSHIRO_GLSL
#define XOSHIRO_GLSL

uint rotl(const uint x, int k)
{
    return (x << k) | (x >> (32 - k));
}

uint nextRandom(inout uvec4 rng) 
{
    const uint result_starstar = rotl(rng[0] * 5, 7) * 9;
	const uint t = rng[1] << 9;

    rng[2] ^= rng[0];
    rng[3] ^= rng[1];
    rng[1] ^= rng[2];
    rng[0] ^= rng[3];

    rng[2] ^= t;
    rng[3] = rotl(rng[3], 11);

    return result_starstar;
}

uvec4 createXoshiro(uvec4 s)
{
    uvec4 state;
    state[0] = s[0];
    state[1] = s[1];
    state[2] = s[2];
    state[3] = s[3];
    return state;
}

#endif // XOSHIRO_GLSL