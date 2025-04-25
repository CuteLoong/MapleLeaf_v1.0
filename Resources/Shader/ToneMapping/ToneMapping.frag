#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#define Linear 0
#define Reinhard 1
#define ReinhardModified 2
#define HejiHableAlu 3
#define HableUc2 4
#define Aces 5

layout(set=0, binding = 0) uniform UniformToneMapping {
	float exposure;
    float gamma;
    float whiteMaxLuminance;
    float whiteScale;
    int type;
} uniformToneMapping;

layout(set=0, binding = 1) uniform sampler2D ResolvedImage;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

float calcLuminance(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 toneMapLinear(vec3 color)
{
    return color;
}

// Reinhard
vec3 toneMapReinhard(vec3 color)
{
    float luminance = calcLuminance(color);
    float reinhard = luminance / (luminance + 1);
    return color * (reinhard / luminance);
}

// Reinhard with maximum luminance
vec3 toneMapReinhardModified(vec3 color)
{
    float luminance = calcLuminance(color);
    float reinhard = luminance * (1 + luminance / (uniformToneMapping.whiteMaxLuminance * uniformToneMapping.whiteMaxLuminance)) * (1 + luminance);
    return color * (reinhard / luminance);
}

// John Hable's ALU approximation of Jim Heji's operator
// http://filmicgames.com/archives/75
vec3 toneMapHejiHableAlu(vec3 color)
{
    color = max(float(0).rrr, color - 0.004);
    color = (color*(6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);

    // Result includes sRGB conversion
    return pow(color, vec3(2.2));
}

// John Hable's Uncharted 2 filmic tone map
// http://filmicgames.com/archives/75
vec3 applyUc2Curve(vec3 color)
{
    float A = 0.22; // Shoulder Strength
    float B = 0.3;  // Linear Strength
    float C = 0.1;  // Linear Angle
    float D = 0.2;  // Toe Strength
    float E = 0.01; // Toe Numerator
    float F = 0.3;  // Toe Denominator

    color = ((color * (A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-(E/F);
    return color;
}

vec3 toneMapHableUc2(vec3 color)
{
    float exposureBias = 2.0f;
    color = applyUc2Curve(exposureBias * color);
    float whiteScale = 1 / applyUc2Curve(vec3(uniformToneMapping.whiteScale)).x;
    color = color * whiteScale;

    return color;
}

vec3 toneMapAces(vec3 color)
{
    // Cancel out the pre-exposure mentioned in
    // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    color *= 0.6;

    float A = 2.51;
    float B = 0.03;
    float C = 2.43;
    float D = 0.59;
    float E = 0.14;

    color = clamp((color*(A*color+B))/(color*(C*color+D)+E), 0.0f, 1.0f);
    return color;
}

vec3 toneMap(vec3 color) {
    switch(uniformToneMapping.type)
    {
        case Linear: return toneMapLinear(color);
        case Reinhard: return toneMapReinhard(color);
        case ReinhardModified: return toneMapReinhardModified(color);
        case HejiHableAlu: return toneMapHejiHableAlu(color);
        case HableUc2: return toneMapHableUc2(color);
        case Aces: return toneMapAces(color);
        default: return color;
    }
}

void main() {
    vec2 uv = vec2(inUV.x, 1.0 - inUV.y);
    vec4 color = texture(ResolvedImage, uv) * uniformToneMapping.exposure;

    vec3 finalColor = color.rgb;

    finalColor = toneMap(finalColor);

    outColor = vec4(pow(finalColor, vec3(1.0f / uniformToneMapping.gamma)), color.a);
}