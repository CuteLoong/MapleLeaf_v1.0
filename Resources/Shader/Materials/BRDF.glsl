#ifndef BRDF_GLSL
#define BRDF_GLSL

#include <Misc/Constants.glsl>
#include "Fresnel.glsl"
#include "Microfacet.glsl"

#define MinCosTheta 1e-6

/** Diffuse
    https://pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Sampling_Reflection_Functions#BSDF::Sample_f
*/
vec2 UniformSampleDisk(const vec2 u)
{
    float r = sqrt(u.x);
    float theta = 2.0f * M_PI * u.y;
    return vec2(r * cos(theta), r * sin(theta));
}

vec2 ConcentricSampleDisk(const vec2 u)
{
    vec2 uOffset = 2.0f * u - vec2(1.0f);
    if (uOffset.x == 0.0f && uOffset.y == 0.0f) return vec2(0.0f);

    float theta, r;
    if (abs(uOffset.x) > abs(uOffset.y))
    {
        r = uOffset.x;
        theta = PiOver4 * (uOffset.y / uOffset.x);
    }
    else
    {
        r = uOffset.y;
        theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
    }

    return r * vec2(cos(theta), sin(theta));
}

vec3 CosineSampleHemisphere(const vec2 u)
{
    vec2 d = ConcentricSampleDisk(u);
    float z = sqrt(max(0.0f, 1.0f - d.x * d.x - d.y * d.y));
    return vec3(d.x, d.y, z);
}

/** Disney's diffuse reflection.
    Based on https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
*/
vec3 DiffuseReflectionDisneyEvalWeight(vec3 diffuseColor, float roughness, vec3 N, vec3 L, vec3 V)
{
    vec3 H = normalize(L + V);
    float VoH = clamp(dot(V, H), 0.0f, 1.0f);
    float LoH = clamp(dot(L, H), 0.0f, 1.0f);

    float VoN = clamp(dot(V, N), 0.0f, 1.0f);
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);

    if(min(VoN, NoL) < MinCosTheta) return vec3(0.0f);

    float Fd90 = 0.5f + 2.0f * LoH * LoH * roughness;
    float Fd0 = 1.0f;

    float wiScatter = evalFresnelSchlick(Fd0, Fd90, VoH);
    float woScatter = evalFresnelSchlick(Fd0, Fd90, LoH);

    return diffuseColor * wiScatter * woScatter;
}


vec3 DiffuseReflectionDisneyEval(vec3 diffuseColor, float roughness, vec3 N, vec3 L, vec3 V)
{
    float VoN = clamp(dot(V, N), 0.0f, 1.0f);
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);

    if(min(VoN, NoL) < MinCosTheta) return vec3(0.0f);

    return DiffuseReflectionDisneyEvalWeight(diffuseColor, roughness, N, L, V) * INV_M_PI * NoL;
}

float DiffuseReflectionDisneyEvalPdf(vec3 V, vec3 N, vec3 L) {
    float NoV = clamp(dot(N, V), 0.0f, 1.0f);
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);

    if(min(NoV, NoL) < MinCosTheta) return 0.0f;

    return NoL * INV_M_PI;
}

/** Specular reflection using microfacets.
*/
vec3 SpecularReflectionMicrofacetEvalWeight(vec3 specularColor, float roughness,vec3 N, vec3 L, vec3 V)
{
    vec3 H = normalize(L + V);
    
    float VoH = clamp(dot(V, H), 0.0f, 1.0f);
    float NoH = clamp(dot(N, H), 0.0f, 1.0f);
    float LoH = clamp(dot(L, H), 0.0f, 1.0f);

    float NoV = clamp(dot(N, V), 0.0f, 1.0f);
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);

    if(min(NoV, NoL) < MinCosTheta) return vec3(0.0f);

    float alpha = roughness * roughness;

    float G = evalMaskingSmithGGXCorrelated(alpha, NoV, NoL);
    vec3 F = evalFresnelSchlick(specularColor, vec3(1.0f), VoH);

    return G * F * VoH / (NoH * NoV); // brdf / pdf
}

vec3 SpecularReflectionMicrofacetEval(vec3 specularColor, float roughness,vec3 N, vec3 L, vec3 V)
{
    vec3 H = normalize(L + V);
    
    float VoH = clamp(dot(V, H), 0.0f, 1.0f);
    float NoH = clamp(dot(N, H), 0.0f, 1.0f);
    float LoH = clamp(dot(L, H), 0.0f, 1.0f);

    float NoV = clamp(dot(N, V), 0.0f, 1.0f);
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);

    if(min(NoV, NoL) < MinCosTheta) return vec3(0.0f);

    float alpha = roughness * roughness;

    float D = evalNdfGGX(alpha, NoH);
    float G = evalMaskingSmithGGXCorrelated(alpha, NoV, NoL);
    vec3 F = evalFresnelSchlick(specularColor, vec3(1.0f), VoH);

    return D * G * F * 0.25f / (/* NoL * */ NoV);
}

float evalPdfReflectionMicrofacet(vec3 V, vec3 N, vec3 L, float roughness) {
    vec3 H = normalize(L + V);
    float NoH = clamp(dot(N, H), 0.0f, 1.0f);
    float NoV = clamp(dot(N, V), 0.0f, 1.0f);
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);
    float VoH = clamp(dot(V, H), 0.0f, 1.0f);

    if(min(NoV, NoL) < MinCosTheta) return 0.0f;

    float alpha = roughness * roughness;

    return evalPdfGGX_NDF(alpha, NoH) / (4.0f * VoH);
}

#endif // BRDF_GLSL