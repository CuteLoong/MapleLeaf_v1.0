#pragma once

#include "Mesh.hpp"

namespace MapleLeaf {
class GPUInstance
{
    friend class GPUScene;

public:
    enum class Status
    {
        None          = 0,
        ModelChanged  = 1,
        MatrixChanged = 2
    };

    struct InstanceData
    {
        glm::mat4 modelMatrix;
        glm::mat4 prevModelMatrix;
        glm::vec3 AABBLocalMin;
        uint32_t  indexCount;
        glm::vec3 AABBLocalMax;
        uint32_t  indexOffset;
        uint32_t  vertexCount;
        uint32_t  vertexOffset;
        uint32_t  instanceID;
        uint32_t  materialID;
        uint32_t  isAreaLight;
        uint32_t  isThin;
        uint32_t  isUpdate;
        uint32_t  padding1;
    };

    GPUInstance() = default;

    explicit GPUInstance(Mesh* mesh, uint32_t instanceID, uint32_t materialID);

    void Update();

    uint32_t     GetInstanceID() const { return instanceData.instanceID; }
    Status       GetInstanceStatus() const { return instanceStatus; }
    InstanceData GetInstanceData() const { return instanceData; }
    Mesh*        GetMesh() const { return mesh; }

    VkDrawIndexedIndirectCommand GetDrawIndexedIndirectCommand() const
    {
        return {instanceData.indexCount, 1, instanceData.indexOffset, int(instanceData.vertexOffset), instanceData.instanceID};
    }

    bool    HasFlag(const std::string& flagName) const;
    Entity* ParentsHasFlagEntity(const std::string& flagName) const;

private:
    Mesh*                  mesh;   // relevant mesh
    std::shared_ptr<Model> model;
    Status                 instanceStatus;

    InstanceData instanceData;

    // offset first is indexOffset, offset second is vertexOffset
    static std::unordered_map<std::shared_ptr<Model>, std::pair<uint32_t, uint32_t>> modelOffset;
    static std::vector<Vertex3D>                                                     verticesArray;
    static std::vector<uint32_t>                                                     indicesArray;
};
}   // namespace MapleLeaf