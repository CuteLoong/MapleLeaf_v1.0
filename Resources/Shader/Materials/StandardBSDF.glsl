#ifndef STANDARD_BSDF_GLSL
#define STANDARD_BSDF_GLSL

#include <Utils/ColorHelpers.glsl>
#include <Materials/Fresnel.glsl>
#include <Materials/DisneyDiffuseBRDF.glsl>
#include <Materials/SpecularMicrofacetBRDF.glsl>

struct StandardBSDFParameters
{
    vec3 diffuse;
    vec3 specular;
    float roughness;
    float metallic;
    float eta;
    vec3 transmission;
    float diffuseTransmission;
    float specularTransmission;

    float pDiffuseReflection;
    float pSpecularReflection;
    float pDiffuseTransmission;
    float pSpecularTransmission;
};


void StandardBSDFInit(inout StandardBSDFParameters params, vec3 diffuse, float specularFactor, float roughness, float metallic, vec3 wi)
{
    params.diffuse = diffuse;
    params.specular = mix(vec3(0.08f) * specularFactor, diffuse, metallic);
    params.roughness = roughness;
    params.metallic = metallic;

    params.eta = 1.5f;
    params.transmission = vec3(0.0f);
    params.diffuseTransmission = 0.0f;
    params.specularTransmission = 0.0f;

    float diffuseWeight = luminance(params.diffuse);
    float dielectricBSDF = (1.0f - metallic) * (1.0f - params.specularTransmission);

    float specularWeight = luminance(evalFresnelSchlick(params.specular, vec3(1.0f), wi.z));
    float metallicBRDF = metallic * (1.0f - params.specularTransmission);

    params.pDiffuseReflection = diffuseWeight * dielectricBSDF * (1.0f - params.diffuseTransmission);
    params.pSpecularReflection = specularWeight * (metallicBRDF + dielectricBSDF);

    params.pDiffuseTransmission = 0.0f;
    params.pSpecularTransmission = 0.0f;

    float normFactor = params.pDiffuseReflection + params.pDiffuseTransmission + params.pSpecularReflection + params.pSpecularTransmission;

    if(normFactor > 0.0f)
    {
        normFactor = 1.0f / normFactor;
        params.pDiffuseReflection *= normFactor;
        params.pSpecularReflection *= normFactor;
        params.pDiffuseTransmission *= normFactor;
        params.pSpecularTransmission *= normFactor;
    }
}

vec3 eval(in StandardBSDFParameters params, vec3 wi, vec3 wo)
{
    vec3 result = vec3(0.0f);
    if(params.pDiffuseReflection > 0.0f)
    {
        result += (1.0f - params.specularTransmission) * (1.0f - params.diffuseTransmission) * DisneyDiffuseBRDF_eval(wi, wo, params.roughness, params.diffuse);
    }
    if(params.pSpecularReflection > 0.0f)
    {
        result += (1.0f - params.specularTransmission) * SpecularMicrofacetBRDF_eval(wi, wo, params.roughness * params.roughness, params.specular);
    }
    return result;
}

bool bsdf_sample(in StandardBSDFParameters params, vec3 u, vec3 wi, out vec3 wo, out float pdf, out vec3 weight)
{
    wo = vec3(0.0f);
    pdf = 0.0f;
    weight = vec3(0.0f);

    bool valid = false;
    float uSelect = u.x;

    if(uSelect < params.pDiffuseReflection) {
        valid = DisneyDiffuseBRDF_sample(vec2(u.y, u.z), wi, params.roughness, params.diffuse, wo, pdf, weight);
        weight /= params.pDiffuseReflection;
        weight *= (1.0f - params.specularTransmission) * (1.0f - params.diffuseTransmission);
        pdf *= params.pDiffuseReflection;

        if(params.pSpecularReflection > 0.f)
            pdf += params.pSpecularReflection * SpecularMicrofacetBRDF_evalPdf(wi, wo, params.roughness * params.roughness);
    }
    else if(uSelect < params.pDiffuseReflection + params.pSpecularReflection) {
        valid = SpecularMicrofacetBRDF_sample(vec2(u.y, u.z), wi, params.roughness * params.roughness, params.specular, wo, pdf, weight);
        weight /= params.pSpecularReflection;
        weight *= (1.0f - params.specularTransmission);
        pdf *= params.pSpecularReflection;

        if(params.pDiffuseReflection > 0.f)
            pdf += params.pDiffuseReflection * DisneyDiffuseBRDF_evalPdf(wi, wo);
    }

    return valid;
}

float evalPdf(in StandardBSDFParameters params, vec3 wi, vec3 wo)
{
    float pdf = 0.0f;

    if(params.pDiffuseReflection > 0.0f)
    {
        pdf += params.pDiffuseReflection * DisneyDiffuseBRDF_evalPdf(wi, wo);
    }
    if(params.pSpecularReflection > 0.0f)
    {
        pdf += params.pSpecularReflection * SpecularMicrofacetBRDF_evalPdf(wi, wo, params.roughness * params.roughness);
    }

    return pdf;
}

# endif // STANDARD_BSDF_GLSL