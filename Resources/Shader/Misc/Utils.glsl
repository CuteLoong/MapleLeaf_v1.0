#ifndef MISC_Utils_GLSL
#define MISC_Utils_GLSL

vec2 UnitVectorToOctahedron(vec3 N)
{
    N.xy /= dot(abs(N), vec3(1.0f));
    if (N.z < 0.0f) N.xy = (1.0f - abs(N.yx)) * vec2(N.x >= 0.0 ? 1.0 : -1.0, N.y >= 0.0 ? 1.0 : -1.0);
    return N.xy;
}

vec3 OctahedronToUnitVector(vec2 Oct)
{
    vec3 N = vec3(Oct, 1 - dot(abs(Oct), vec2(1.0f)));
    if (N.z < 0.0f) N.xy = (1.0 - abs(N.yx)) * vec2(N.x >= 0.0 ? 1.0 : -1.0, N.y >= 0.0 ? 1.0 : -1.0);
    return normalize(N);
}

float CompressUVToF32(vec2 uv)
{
    uint u12_x = uint(uv.x * 4095.0);
    uint u12_y = uint(uv.y * 4095.0);

    uint compressed = (u12_x << 12) | u12_y; 
    return uintBitsToFloat(compressed);  
}
vec2 DecompressF32ToUV(float data)
{
    uint v = floatBitsToUint(data); 
    return vec2(float((v >> 12) & 0xFFF) / 4095.0, 
                float(v & 0xFFF) / 4095.0);
}

uint PackVec2ToUint(vec2 v) {
    uvec2 iv = uvec2((v * 32767.0) + 32767.0); // 量化到 16-bit
    return (iv.x & 0xFFFFu) | (iv.y << 16);    // x 存低 16 位，y 存高 16 位
}
vec2 UnpackUintToVec2(uint data) {
    uvec2 iv = uvec2(data & 0xFFFFu, data >> 16); // 拆分回 16-bit 值
    return (vec2(iv) - 32767.0) / 32767.0;       // 还原到原始范围
}

uint floatToE4M3(float f) {
    uint bits = floatBitsToUint(f);
    uint sign = bits >> 31;
    uint exponent = (bits >> 23) & 0xFF;
    uint mantissa = bits & 0x007FFFFF;

    if (exponent == 0) return 0;
    
    int exp32 = int(exponent) - 127;
    int exp8 = exp32 + 7;
    exp8 = clamp(exp8, 0, 15);

    uint mantissa8 = (mantissa >> (23 - 3)) & 0x7;
    uint result = (sign << 7) | (exp8 << 3) | mantissa8;
    return result;
}

uint packE4M3x2(vec2 v) {
    uint x = floatToE4M3(v.x);
    uint y = floatToE4M3(v.y);
    return (y << 8) | x;
}

float e4m3ToFloat(uint fp8)
{
    uint sign = (fp8 >> 7) & 0x1;
    uint exp8 = (fp8 >> 3) & 0xF;
    uint mantissa = fp8 & 0x7;

    uint exponent = exp8 - 7 + 127;
    uint mantissa32 = mantissa << (23 - 3);
    uint result = (sign << 31) | (exponent << 23) | mantissa32;
    return uintBitsToFloat(result);
} 

vec2 unpackE4M3x2(uint packed) {
    float x = e4m3ToFloat(packed & 0xFF);
    float y = e4m3ToFloat((packed >> 8) & 0xFF);
    return vec2(x, y);
}
#endif