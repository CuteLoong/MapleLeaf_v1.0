#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

namespace MapleLeaf {
#define M_PI 3.14159265358979323846
class Maths
{
public:
    /**
     * Generate a vector that is orthogonal to the input vector
     * This can be used to invent a tangent frame for meshes that don't have real tangents/bitangents.
     * @param[in] u Unit vector.
     * @return v Unit vector that is orthogonal to u.
     */
    static glm::vec3 perp_stark(const glm::vec3& u)
    {
        // TODO: Validate this and look at numerical precision etc. Are there better ways to do it?
        glm::vec3 a   = glm::abs(u);
        uint32_t  uyx = (a.x - a.y) < 0 ? 1 : 0;
        uint32_t  uzx = (a.x - a.z) < 0 ? 1 : 0;
        uint32_t  uzy = (a.y - a.z) < 0 ? 1 : 0;
        uint32_t  xm  = uyx & uzx;
        uint32_t  ym  = (1 ^ xm) & uzy;
        uint32_t  zm  = 1 ^ (xm | ym);   // 1 ^ (xm & ym)
        glm::vec3 v   = glm::normalize(glm::cross(u, glm::vec3(xm, ym, zm)));
        return v;
    }

    /**
     * Combines a seed into a hash and modifies the seed by the new hash.
     * @param seed The seed.
     * @param v The value to hash.
     */
    template<typename T>
    static void HashCombine(std::size_t& seed, const T& v) noexcept
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<class integral>
    static constexpr integral AlignUp(integral x, size_t a) noexcept
    {
        return integral((x + (integral(a) - 1)) & ~integral(a - 1));
    }

    // Base 2 Van der Corput radical inverse
    static float radicalInverse(uint32_t i)
    {
        i = (i & 0x55555555) << 1 | (i & 0xAAAAAAAA) >> 1;
        i = (i & 0x33333333) << 2 | (i & 0xCCCCCCCC) >> 2;
        i = (i & 0x0F0F0F0F) << 4 | (i & 0xF0F0F0F0) >> 4;
        i = (i & 0x00FF00FF) << 8 | (i & 0xFF00FF00) >> 8;
        i = (i << 16) | (i >> 16);
        return float(i) * 2.3283064365386963e-10f;
    }

    static glm::vec3 hammersleyUniform(uint32_t i, uint32_t n)
    {
        glm::vec2 uv((float)i / (float)n, radicalInverse(i));

        // Map to radius 1 hemisphere
        float phi = uv.y * 2.0f * (float)M_PI;
        float t   = 1.0f - uv.x;
        float s   = std::sqrt(1.0f - t * t);
        return glm::vec3(s * std::cos(phi), s * std::sin(phi), t);
    }

    inline glm::vec3 hammersleyCosine(uint32_t i, uint32_t n)
    {
        glm::vec2 uv((float)i / (float)n, radicalInverse(i));

        // Map to radius 1 hemisphere
        float phi = uv.y * 2.0f * (float)M_PI;
        float t   = std::sqrt(1.0f - uv.x);
        float s   = std::sqrt(1.0f - t * t);
        return glm::vec3(s * std::cos(phi), s * std::sin(phi), t);
    }
};
}   // namespace MapleLeaf