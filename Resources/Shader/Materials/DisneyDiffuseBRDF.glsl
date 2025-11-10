#ifndef DISNEY_DIFFUSE_BRDF_GLSL
#define DISNEY_DIFFUSE_BRDF_GLSL

#include <Misc/Constants.glsl>
#include <Utils/MathHelpers.glsl>
#include <Materials/Fresnel.glsl>

// Returns f(wi, wo) * pi.
vec3 DisneyDiffuseBRDF_evalWeight(vec3 wi, vec3 wo, float roughness, vec3 albedo)
{
    vec3 h = normalize(wi + wo);
    float woDotH = max(dot(wo, h), 0.0f);
    float fd90 = 0.5f + 2.0f * woDotH * woDotH * roughness;
    float fd0 = 1.0f;
    float wiScatter = evalFresnelSchlick(fd90, fd0, wi.z);
    float woScatter = evalFresnelSchlick(fd90, fd0, wo.z);
    return albedo * wiScatter * woScatter;
}

vec3 DisneyDiffuseBRDF_eval(const vec3 wi, const vec3 wo, float roughness, vec3 albedo)
{
    if (min(wi.z, wo.z) < kMinCosTheta)
        return vec3(0.0f);
    
    return DisneyDiffuseBRDF_evalWeight(wi, wo, roughness, albedo) * INV_M_PI * wo.z;
}

// Sample weight f(wi, wo) * dot(wo, n) / pdf(wo).
bool DisneyDiffuseBRDF_sample(vec2 u, const vec3 wi, float roughness, vec3 albedo, out vec3 wo, out float pdf, out vec3 weight)
{
    wo = sample_cosine_hemisphere_concentric(u, pdf);

    if(min(wi.z, wo.z) < kMinCosTheta)
    {
        weight = vec3(0.0f);
        return false;
    }

    weight = DisneyDiffuseBRDF_evalWeight(wi, wo, roughness, albedo);
    return true;
}

float DisneyDiffuseBRDF_evalPdf(const vec3 wi, const vec3 wo)
{
    if(min(wi.z, wo.z) < kMinCosTheta)
        return 0.0f;

    return INV_M_PI * wo.z;
}

#endif // DISNEY_DIFFUSE_BRDF_GLSL