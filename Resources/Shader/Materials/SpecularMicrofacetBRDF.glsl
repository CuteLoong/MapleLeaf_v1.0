#ifndef SPECULAR_MICROFACET_BRDF_GLSL
#define SPECULAR_MICROFACET_BRDF_GLSL

#include <Misc/Constants.glsl>
#include <Utils/MathHelpers.glsl>
#include <Materials/Fresnel.glsl>
#include <Materials/Microfacet.glsl>

const float kMinGGXAlpha = 0.0064f;

vec3 SpecularMicrofacetBRDF_eval(vec3 wi, vec3 wo, float alpha, vec3 albedo)
{
    if(min(wi.z, wo.z) < kMinCosTheta)
        return vec3(0.0f);
    
    vec3 h = normalize(wi + wo);
    float wiDotH = max(dot(wi, h), 0.0f);

    float D = evalNdfGGX(alpha, h.z);
    float G = evalMaskingSmithGGXCorrelated(alpha, wi.z, wo.z);
    vec3 F = evalFresnelSchlick(albedo, vec3(1.0f), wiDotH);

    return vec3(D * G * F * 0.25f / wi.z);
}

bool SpecularMicrofacetBRDF_sample(vec2 u, const vec3 wi, float alpha, vec3 albedo, out vec3 wo, out float pdf, out vec3 weight)
{
    wo = vec3(0.0f);
    pdf = 0.0f;
    weight = vec3(0.0f);

    if(wi.z < kMinCosTheta)
        return false;

    if(alpha < kMinGGXAlpha) {
        wo = vec3(-wi.x, -wi.y, wi.z);
        pdf = 0.f;
        weight = evalFresnelSchlick(albedo, vec3(1.0f), wi.z);
        return true;
    }

    vec3 h = sampleGGX_NDF(alpha, u, pdf);

    float wiDotH = max(dot(wi, h), 0.0f);
    wo = 2.0f * wiDotH * h - wi;
    if(wo.z < kMinCosTheta)
        return false;

    float G = evalMaskingSmithGGXCorrelated(alpha, wi.z, wo.z);
    // float GOverG1wo = G * (1.f + evalLambdaGGX(alpha * alpha, wi.z));
    vec3 F = evalFresnelSchlick(albedo, vec3(1.0f), wiDotH);

    pdf /= (4.f * wiDotH); // Jacobian of the reflection operator.
    weight = F * G * wiDotH / (wi.z * h.z);

    return true;
}

float SpecularMicrofacetBRDF_evalPdf(const vec3 wi, const vec3 wo, float alpha)
{
    if(min(wi.z, wo.z) < kMinCosTheta)
        return 0.0f;

    vec3 h = normalize(wi + wo);
    float wiDotH = max(dot(wi, h), 0.0f);
    float pdf = evalPdfGGX_NDF(alpha, h.z);
    return pdf / (4.0f * wiDotH);
}

#endif // SPECULAR_MICROFACET_BRDF_GLSL