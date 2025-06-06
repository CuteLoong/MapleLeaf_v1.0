#pragma once

#include "Buffer.hpp"
#include "Resource.hpp"
#include "Vertex.hpp"
#include "glm/glm.hpp"
#include <stdint.h>
#include <vector>

#include "config.h"

namespace MapleLeaf {
struct BLASInput
{
    VkAccelerationStructureGeometryKHR       geometry;
    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;
    VkBuildAccelerationStructureFlagsKHR     flags{0};

    BLASInput() = default;

    BLASInput(const VkAccelerationStructureGeometryKHR& geometry, const VkAccelerationStructureBuildRangeInfoKHR& buildRangeInfo)
        : geometry(geometry)
        , buildRangeInfo(buildRangeInfo)
    {}
};

class Model : public Resource
{
public:
    enum class Status
    {
        None,
        CPU_BINDING,
        GPU_BINDING
    };

    /**
     * Creates a new empty model.
     */
    Model() = default;

    /**
     * Creates a new model.
     * @param vertices The model vertices.
     * @param indices The model indices.
     */
    explicit Model(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices = {});

    bool CmdRender(const CommandBuffer& commandBuffer, uint32_t instances = 1);

    std::type_index GetTypeIndex() const override { return typeid(Model); }

    void                         SetVertices(const std::vector<Vertex3D>& vertices);
    void                         SetIndices(const std::vector<uint32_t>& indices);
    const std::vector<Vertex3D>& GetVertices(std::size_t offset = 0) const { return vertices; }
    const std::vector<uint32_t>& GetIndices(std::size_t offset = 0) const { return indices; };

    BLASInput* GetBLASInput() const { return blasInput.get(); }

    const glm::vec3&                GetMinExtents() const { return minExtents; }
    const glm::vec3&                GetMaxExtents() const { return maxExtents; }
    const std::array<glm::mat4, 6>& GetMappingViews() const { return mappingViews; }
    const std::array<glm::mat4, 6>& GetMappingOrthos() const { return mappingOrthos; }

    float              GetWidth() const { return maxExtents.x - minExtents.x; }
    float              GetHeight() const { return maxExtents.y - minExtents.y; }
    float              GetDepth() const { return maxExtents.z - minExtents.z; }
    float              GetRadius() const { return radius; }
    const Buffer*      GetVertexBuffer() const { return vertexBuffer.get(); }
    const Buffer*      GetIndexBuffer() const { return indexBuffer.get(); }
    uint32_t           GetVertexCount() const { return vertexCount; }
    uint32_t           GetIndexCount() const { return indexCount; }
    static VkIndexType GetIndexType() { return VK_INDEX_TYPE_UINT32; }

    bool IsThin() const { return isThin; }

protected:
    void Initialize(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices = {});
    void BindInfoToGPU();
    void InitBLASInput();

private:
    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;

    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;

    std::unique_ptr<BLASInput> blasInput;

    uint32_t vertexCount = 0;
    uint32_t indexCount  = 0;
    Status   status      = Status::None;

    glm::vec3 minExtents;
    glm::vec3 maxExtents;

    std::array<glm::mat4, 6> mappingViews;
    std::array<glm::mat4, 6> mappingOrthos;

    float radius = 0.0f;
    bool  isThin = false;
};
}   // namespace MapleLeaf