#pragma once

#include "Transform.hpp"
#include <string>
#include <vector>

namespace MapleLeaf {
class NodeID
{
public:
    struct NodeIDHash
    {
        std::size_t operator()(const NodeID& nodeID) const { return std::hash<uint32_t>()(nodeID.get()); }
    };

    NodeID() { index = 0xFFFFFFFF; }
    NodeID(uint32_t index)
        : index(index)
    {}

    uint32_t get() const { return index; }
    bool     isValid() const { return index != 0xFFFFFFFF; }

    operator uint32_t() const { return index; }
    NodeID operator=(const NodeID rhs)
    {
        index = rhs;
        return *this;
    }

    static constexpr uint32_t kInvalidID = 0xFFFFFFFF;
    static NodeID             Invalid() { return NodeID(kInvalidID); }

    friend bool operator==(const NodeID& lhs, const NodeID& rhs) { return lhs.index == rhs.index; }

private:
    uint32_t index;
};

class SceneNode
{
public:
    SceneNode() = default;

    std::string              name;
    Transform*               transform;   // auto free by unique_ptr in component
    NodeID                   parent;
    std::vector<NodeID>      children;
    std::vector<uint32_t>    meshes;
    std::vector<std::string> flags;
};

using SceneGraph = std::vector<SceneNode>;
}   // namespace MapleLeaf