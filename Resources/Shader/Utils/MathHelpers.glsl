#ifndef MATH_HELPERS_GLSL
#define MATH_HELPERS_GLSL

#include <Misc/Constants.glsl>

/**
 * Generate a vector that is orthogonal to the input vector.
 * This can be used to invent a tangent frame for meshes that don't have real tangents/bitangents.
 * @param[in] u Unit vector.
 * @return v Unit vector that is orthogonal to u.
 */
vec3 perp_stark(vec3 u)
{
    // TODO: Validate this and look at numerical precision etc. Are there better ways to do it?
    vec3 a = abs(u);
    uint uyx = (a.x - a.y) < 0 ? 1 : 0;
    uint uzx = (a.x - a.z) < 0 ? 1 : 0;
    uint uzy = (a.y - a.z) < 0 ? 1 : 0;
    uint xm = uyx & uzx;
    uint ym = (1 ^ xm) & uzy;
    uint zm = 1 ^ (xm | ym); // 1 ^ (xm & ym)
    vec3 v = normalize(cross(u, vec3(xm, ym, zm)));
    return v;
}

/**
 * Uniform sampling of the unit disk using Shirley's concentric mapping.
 * @param[in] u Uniform random numbers in [0,1)^2.
 * @return Sampled point on the unit disk.
 */
vec2 sample_disk_concentric(vec2 u)
{
    u = 2.f * u - 1.f;
    if (u.x == 0.f && u.y == 0.f)
        return u;
    float phi, r;
    if (abs(u.x) > abs(u.y))
    {
        r = u.x;
        phi = (u.y / u.x) * M_PI_4;
    }
    else
    {
        r = u.y;
        phi = M_PI_2 - (u.x / u.y) * M_PI_4;
    }
    return r * vec2(cos(phi), sin(phi));
}

/**
 * Cosine-weighted sampling of the hemisphere using Shirley's concentric mapping.
 * @param[in] u Uniform random numbers in [0,1)^2.
 * @param[out] pdf Probability density of the sampled direction (= cos(theta)/pi).
 * @return Sampled direction in the local frame (+z axis up).
 */
vec3 sample_cosine_hemisphere_concentric(vec2 u, out float pdf)
{
    vec2 d = sample_disk_concentric(u);
    float z = sqrt(max(0.f, 1.f - dot(d, d)));
    pdf = z * M_1_PI;
    return vec3(d, z);
}

/**
 * Uniform sampling of a triangle.
 * @param[in] u Uniform random numbers in [0,1)^2.
 * @return Barycentric coordinates (1-u-v,u,v) of the sampled point.
 */
vec3 sample_triangle(vec2 u)
{
    float su = sqrt(u.x);
    vec2 b = vec2(1.f - su, u.y * su);
    return vec3(1.f - b.x - b.y, b.x, b.y);
}

/**
 * Sample from the von-Mises Fisher distribution.
 * @param[in] u Uniform random numbers in [0,1)^2.
 * @param[in] kappa VMF concentration parameter.
 * @return Sampled direction in the local frame (+z axis up).
 */
vec3 sampleVonMisesFisher(vec2 u, float kappa)
{
    float sy = max(1.f - u.y, 1e-6f);
    float phi = M_2PI * u.x;
    float cosTheta = 1.f + log(exp(-2.f * kappa) * (1.f - sy) + sy) / kappa;
    float sinTheta = sqrt(1.f - cosTheta * cosTheta);
    vec3 d = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    return d;
}

#endif // MATH_HELPERS_GLSL