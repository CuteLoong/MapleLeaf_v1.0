#pragma once

#include "ASScene.hpp"
#include "DescriptorHandler.hpp"
#include "GPUInstance.hpp"
#include "GPUMaterial.hpp"
#include "Scene.hpp"
#include "StorageBuffer.hpp"

namespace MapleLeaf {
class GPUScene : public DerivedScene
{
    friend class Scene;
    friend class ASScene;

public:
    enum class UpdateStatus
    {
        NoneChanged = 0,
        InstanceChanged,
        AllChanged
    };

    GPUScene();

    ~GPUScene();

    void Start() override;
    void Update() override;

    void SetVertices(const std::vector<Vertex3D>& vertices);
    void SetIndices(const std::vector<uint32_t>& indices);

    void PushDescriptors(DescriptorsHandler& descriptorSet, bool DrawCulling = true);
    bool CmdRender(const CommandBuffer& commandBuffer, bool DrawCulling = true);

    std::vector<GPUInstance>& GetInstances() { return instances; }
    std::vector<GPUMaterial>& GetMaterials() { return materials; }

    const Buffer* GetVertexBuffer() const { return vertexBuffer.get(); }
    const Buffer* GetIndexBuffer() const { return indexBuffer.get(); }

    const StorageBuffer* GetInstanceDatasHandler() const { return instancesBuffer.get(); }
    const StorageBuffer* GetMaterialDatasHandler() const { return materialsBuffer.get(); }
    const StorageBuffer* GetEmissiveIDBuffer() const { return emissiveIDBuffer.get(); }
    const StorageBuffer* GetEmissiveTriangleBuffer() const { return emissiveTrianglesBuffer.get(); }

    const IndirectBuffer* GetIndirectBuffer() const { return drawCullingIndirectBuffer.get(); }

    uint32_t     GetEmissiveCount() const { return emissiveIDArray.size(); }
    uint32_t     GetEmissiveTriangleCount() const { return emissiveTrianglesDatas.size(); }
    uint32_t     GetInstanceCount() const { return instances.size(); }
    UpdateStatus GetUpdateStatus() const { return updateStatus; }

private:
    std::vector<GPUInstance> instances;
    std::vector<GPUMaterial> materials;

    std::vector<int> emissiveIDArray;

    std::unique_ptr<Buffer>                    vertexBuffer;
    std::unique_ptr<Buffer>                    indexBuffer;
    std::vector<GPUInstance::InstanceData>     instancesDatas;
    std::vector<GPUMaterial::MaterialData>     materialsDatas;
    std::vector<GPUInstance::EmissiveTriangle> emissiveTrianglesDatas;
    std::vector<VkDrawIndexedIndirectCommand>  drawAllMeshCommands;

    std::unique_ptr<StorageBuffer> instancesBuffer;
    std::unique_ptr<StorageBuffer> materialsBuffer;
    std::unique_ptr<StorageBuffer> emissiveTrianglesBuffer;
    std::unique_ptr<StorageBuffer> emissiveIDBuffer;

    std::unique_ptr<IndirectBuffer> drawCullingIndirectBuffer;
    std::unique_ptr<IndirectBuffer> drawAllMeshIndirectBuffer;

    UpdateStatus updateStatus;
};
}   // namespace MapleLeaf