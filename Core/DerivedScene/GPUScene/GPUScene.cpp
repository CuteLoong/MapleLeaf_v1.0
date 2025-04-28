#include "GPUScene.hpp"
#include "Scenes.hpp"
#include "StorageBuffer.hpp"

#include "config.h"

namespace MapleLeaf {
GPUScene::GPUScene() {}

GPUScene::~GPUScene()
{   // Static class variables must be freed manually
    GPUMaterial::materialArray.clear();
    GPUMaterial::images.clear();
    GPUInstance::indicesArray.clear();
    GPUInstance::verticesArray.clear();
    GPUInstance::modelOffset.clear();
}

void GPUScene::Start()
{
    const auto& meshes = Scenes::Get()->GetScene()->GetComponents<Mesh>();

    for (const auto& mesh : meshes) {
        const auto& material = mesh->GetMaterial();
        if (GPUMaterial::GetMaterialID(material)) continue;

        materials.push_back(GPUMaterial(material));
    }

    for (const auto& mesh : meshes) {
        const auto& material = mesh->GetMaterial();
        instances.push_back(GPUInstance(mesh, mesh->GetInstanceId(), GPUMaterial::GetMaterialID(material).value()));
    }

    for (auto& instance : instances) {
        instancesDatas.push_back(instance.GetInstanceData());
        drawAllMeshCommands.push_back(instance.GetDrawIndexedIndirectCommand());
    }

    for (auto& material : materials) {
        const auto& materialData = material.GetMaterialData();
        materialsDatas.push_back(material.GetMaterialData());
    }

    SetIndices(GPUInstance::indicesArray);
    SetVertices(GPUInstance::verticesArray);

    instancesBuffer = std::make_unique<StorageBuffer>(sizeof(GPUInstance::InstanceData) * instancesDatas.size(), instancesDatas.data());
    materialsBuffer = std::make_unique<StorageBuffer>(sizeof(GPUMaterial::MaterialData) * materialsDatas.size(), materialsDatas.data());

    drawCullingIndirectBuffer = std::make_unique<IndirectBuffer>(instances.size() * sizeof(VkDrawIndexedIndirectCommand));
    drawAllMeshIndirectBuffer =
        std::make_unique<IndirectBuffer>(drawAllMeshCommands.size() * sizeof(VkDrawIndexedIndirectCommand), drawAllMeshCommands.data());

    updateStatus = UpdateStatus::AllChanged;
}

// Now only instance alter, e.g. instance update but not add or remove
// TODO UpdateMaterial and instance Add or Delete
void GPUScene::Update()
{
#ifdef MAPLELEAF_GPUSCENE_DEBUG
    auto debugStart = Time::Now();
#endif
    GPUInstance::Status InstanceUpdateStatus = GPUInstance::Status::None;

    for (uint32_t i = 0; i < instances.size(); i++) {
        GPUInstance& instance = instances[i];
        instance.Update();

        const auto& status   = instance.GetInstanceStatus();
        InstanceUpdateStatus = std::max(InstanceUpdateStatus, status);

        switch (status) {
        case GPUInstance::Status::ModelChanged: drawAllMeshCommands[i] = instance.GetDrawIndexedIndirectCommand();
        case GPUInstance::Status::MatrixChanged: instancesDatas[i] = instance.GetInstanceData(); break;
        case GPUInstance::Status::None: break;
        default: break;
        }
    }

    switch (InstanceUpdateStatus) {
    case GPUInstance::Status::ModelChanged:
        SetIndices(GPUInstance::indicesArray);
        SetVertices(GPUInstance::verticesArray);
        instancesBuffer->Update(instancesDatas.data(), sizeof(GPUInstance::InstanceData) * instancesDatas.size());
        materialsBuffer->Update(materialsDatas.data(), sizeof(GPUMaterial::MaterialData) * materialsDatas.size());
        drawCullingIndirectBuffer->Update(drawAllMeshCommands.data(), drawAllMeshCommands.size() * sizeof(VkDrawIndexedIndirectCommand));
        drawAllMeshIndirectBuffer->Update(drawAllMeshCommands.data(), drawAllMeshCommands.size() * sizeof(VkDrawIndexedIndirectCommand));
        updateStatus = UpdateStatus::AllChanged;
        break;
    case GPUInstance::Status::MatrixChanged:
        instancesBuffer->Update(instancesDatas.data(), sizeof(GPUInstance::InstanceData) * instancesDatas.size());
        materialsBuffer->Update(materialsDatas.data(), sizeof(GPUMaterial::MaterialData) * materialsDatas.size());
        updateStatus = UpdateStatus::InstanceChanged;
        break;
    case GPUInstance::Status::None: updateStatus = UpdateStatus::NoneChanged; break;
    default: break;
    }
#ifdef MAPLELEAF_GPUSCENE_DEBUG
    Log::Out("Update GPU Scene: ", (Time::Now() - debugStart).AsMilliseconds<float>(), "ms\n");
    debugStart = Time::Now();
#endif
}

void GPUScene::PushDescriptors(DescriptorsHandler& descriptorSet, bool DrawCulling)
{
    descriptorSet.Push("instanceDatas", instancesBuffer);
    descriptorSet.Push("materialDatas", materialsBuffer);
    if (DrawCulling)
        descriptorSet.Push("drawCommandBuffer", *drawCullingIndirectBuffer);
    else
        descriptorSet.Push("drawCommandBuffer", *drawAllMeshIndirectBuffer);

    for (int i = 0; i < GPUMaterial::images.size(); i++) {
        descriptorSet.Push("ImageSamplers", GPUMaterial::images[i], i);
    }
}

bool GPUScene::CmdRender(const CommandBuffer& commandBuffer, bool DrawCulling)
{
    // if ((*DrawCulling) == nullptr) return false;
    uint32_t drawCount = DrawCulling ? drawCullingIndirectBuffer->GetSize() / sizeof(VkDrawIndexedIndirectCommand)
                                     : drawAllMeshIndirectBuffer->GetSize() / sizeof(VkDrawIndexedIndirectCommand);

    if (vertexBuffer && indexBuffer && DrawCulling) {
        VkBuffer     vertexBuffers[1] = {vertexBuffer->GetBuffer()};
        VkDeviceSize offsets[1]       = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirect(commandBuffer, drawCullingIndirectBuffer->GetBuffer(), 0, drawCount, sizeof(VkDrawIndexedIndirectCommand));
    }
    else if (vertexBuffer && indexBuffer && !DrawCulling) {
        VkBuffer     vertexBuffers[1] = {vertexBuffer->GetBuffer()};
        VkDeviceSize offsets[1]       = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirect(commandBuffer, drawAllMeshIndirectBuffer->GetBuffer(), 0, drawCount, sizeof(VkDrawIndexedIndirectCommand));
    }
    else {
        return false;
    }

    return true;
}

void GPUScene::SetVertices(const std::vector<Vertex3D>& vertices)
{
    vertexBuffer = nullptr;

    if (vertices.empty()) return;

    Buffer vertexStaging(sizeof(Vertex3D) * vertices.size(),
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         vertices.data());
    vertexBuffer = std::make_unique<Buffer>(
        vertexStaging.GetSize(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CommandBuffer commandBuffer;

    VkBufferCopy copyRegion = {};
    copyRegion.size         = vertexStaging.GetSize();
    vkCmdCopyBuffer(commandBuffer, vertexStaging.GetBuffer(), vertexBuffer->GetBuffer(), 1, &copyRegion);

    commandBuffer.SubmitIdle();
}

void GPUScene::SetIndices(const std::vector<uint32_t>& indices)
{
    indexBuffer = nullptr;

    if (indices.empty()) return;

    Buffer indexStaging(sizeof(uint32_t) * indices.size(),
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        indices.data());
    indexBuffer = std::make_unique<Buffer>(
        indexStaging.GetSize(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CommandBuffer commandBuffer;

    VkBufferCopy copyRegion = {};
    copyRegion.size         = indexStaging.GetSize();
    vkCmdCopyBuffer(commandBuffer, indexStaging.GetBuffer(), indexBuffer->GetBuffer(), 1, &copyRegion);

    commandBuffer.SubmitIdle();
}
}   // namespace MapleLeaf