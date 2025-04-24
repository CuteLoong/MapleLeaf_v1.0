#pragma once

#include "AccelerationStruct.hpp"
#include "DerivedScene.hpp"

#include "StorageBuffer.hpp"
#include "config.h"

namespace MapleLeaf {
class ASScene : public DerivedScene
{
    friend class Scene;

public:
    ASScene();
    ~ASScene();

    void Start();
    void Update();

    // Build soft acceleration structure
    using AABB = std::pair<glm::vec3, glm::vec3>;
    struct BVHNode
    {
        static const uint32_t InvalidMask = 0xFFFFFFFF;

        glm::vec3 min;
        uint32_t  instanceID = InvalidMask;   // or proxyID
        glm::vec3 max;
        uint32_t  next = InvalidMask;

        BVHNode() = default;

        void SetBounds(const AABB& bounds)
        {
            this->min = bounds.first;
            this->max = bounds.second;
        }

        bool IsLeaf() const { return instanceID != InvalidMask; }
    };


    struct BVHPackedNode
    {
        uint32_t a, b, c, d;
    };

    void BuildBVH(bool update = false);

    const std::vector<BVHNode>& GetBVHNodes() const { return bvhNodes; }
    const StorageBuffer*        GetBVHBuffer() const { return bvhBuffer.get(); }

    // Vk hardware raytracing acceleration structure, bottom level acceleration structure, top level acceleration structure
    void BuildBLAS(VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, bool update = false);
    void BuildTLAS(VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, bool update = false);

    const std::vector<std::unique_ptr<AccelerationStruct>>& GetBottomLevelAccelerationStructs() const { return bottomLevelaccelerationStructs; }
    const std::unique_ptr<AccelerationStruct>&              GetBottomLevelAccelerationStruct(uint32_t index) const
    {
        return bottomLevelaccelerationStructs[index];
    }
    const std::unique_ptr<AccelerationStruct>& GetTopLevelAccelerationStruct() const { return topLevelAccelerationStruct; }

    inline VkTransformMatrixKHR ToTransformMatrixKHR(glm::mat4 matrix);

private:
    std::vector<std::unique_ptr<AccelerationStruct>> bottomLevelaccelerationStructs;
    std::unique_ptr<AccelerationStruct>              topLevelAccelerationStruct;

    std::vector<BVHNode>           bvhNodes;
    std::unique_ptr<StorageBuffer> bvhBuffer;
    std::vector<BVHPackedNode>     bvhPackedNodes;
};
}   // namespace MapleLeaf