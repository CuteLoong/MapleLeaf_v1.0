#pragma once

#include "Maths.hpp"
#include "Shader.hpp"
#include "glm/glm.hpp"
#include <stdint.h>

namespace MapleLeaf {
class Vertex2D
{
public:
    Vertex2D() = default;
    Vertex2D(const glm::vec2& position, const glm::vec2& uv)
        : position(position)
        , uv(uv)
    {}

    static Shader::VertexInput GetVertexInput(uint32_t baseBinding = 0)
    {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions = {{baseBinding, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX}};

        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
            {0, baseBinding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, position)},
            {1, baseBinding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, uv)}};
        return {bindingDescriptions, attributeDescriptions};
    }

    bool operator==(const Vertex2D& rhs) const { return position == rhs.position && uv == rhs.uv; }

    bool operator!=(const Vertex2D& rhs) const { return !operator==(rhs); }

    glm::vec2 position;
    glm::vec2 uv;
};

class Vertex3D
{
public:
    Vertex3D() = default;
    Vertex3D(const glm::vec3& position, const glm::vec2& uv, const glm::vec3& normal, const glm::vec3& tangent)
        : position(position)
        , uv(uv)
        , normal(normal)
        , tangent(tangent)
    {}

    static Shader::VertexInput GetVertexInput(uint32_t baseBinding = 0)
    {
        std::vector<VkVertexInputBindingDescription>   bindingDescriptions   = {{baseBinding, sizeof(Vertex3D), VK_VERTEX_INPUT_RATE_VERTEX}};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
            {0, baseBinding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, position)},
            {1, baseBinding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex3D, uv)},
            {2, baseBinding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, normal)},
            {3, baseBinding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, tangent)}};
        return {bindingDescriptions, attributeDescriptions};
    }

    bool operator==(const Vertex3D& rhs) const { return position == rhs.position && uv == rhs.uv && normal == rhs.normal && tangent == rhs.tangent; }

    bool operator!=(const Vertex3D& rhs) const { return !operator==(rhs); }

    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 tangent;
};
}   // namespace MapleLeaf

namespace std {
template<>
struct hash<MapleLeaf::Vertex2D>
{
    size_t operator()(const MapleLeaf::Vertex2D& vertex) const noexcept
    {
        size_t seed = 0;
        MapleLeaf::Maths::HashCombine(seed, vertex.position);
        MapleLeaf::Maths::HashCombine(seed, vertex.uv);
        return seed;
    }
};

template<>
struct hash<MapleLeaf::Vertex3D>
{
    size_t operator()(const MapleLeaf::Vertex3D& vertex) const noexcept
    {
        size_t seed = 0;
        MapleLeaf::Maths::HashCombine(seed, vertex.position);
        MapleLeaf::Maths::HashCombine(seed, vertex.uv);
        MapleLeaf::Maths::HashCombine(seed, vertex.normal);
        MapleLeaf::Maths::HashCombine(seed, vertex.tangent);
        return seed;
    }
};
}   // namespace std