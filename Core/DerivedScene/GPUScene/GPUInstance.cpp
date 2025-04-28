#include "GPUInstance.hpp"

#include "AnimationController.hpp"
#include "Entity.hpp"
#include "Light.hpp"
#include "Transform.hpp"

namespace MapleLeaf {
std::unordered_map<std::shared_ptr<Model>, std::pair<uint32_t, uint32_t>> GPUInstance::modelOffset{};
std::vector<Vertex3D>                                                     GPUInstance::verticesArray{};
std::vector<uint32_t>                                                     GPUInstance::indicesArray{};

GPUInstance::GPUInstance(Mesh* mesh, uint32_t instanceID, uint32_t materialID)
    : mesh(mesh)
    , instanceStatus(Status::ModelChanged)
{
    model = mesh->GetModel();

    instanceData.instanceID = instanceID;
    instanceData.materialID = materialID;

    instanceData.modelMatrix     = mesh->GetEntity()->GetComponent<Transform>()->GetWorldMatrix();
    instanceData.prevModelMatrix = instanceData.modelMatrix;   // maybe not correct, but the first frame is not important
    instanceData.AABBLocalMin    = model->GetMinExtents();
    instanceData.AABBLocalMax    = model->GetMaxExtents();
    instanceData.indexCount      = model->GetIndexCount();
    instanceData.vertexCount     = model->GetVertexCount();
    instanceData.isThin          = model->IsThin();
    instanceData.isUpdate        = 0;
    instanceData.isAreaLight     = 0;

    Entity* entity = mesh->GetEntity();
    while (entity != nullptr) {
        instanceData.isUpdate = entity->GetComponent<AnimationController>() != nullptr;
        entity                = entity->GetParent();
    }

    if (const auto light = mesh->GetEntity()->GetComponent<Light>(); light != nullptr && light->type == LightType::Area) instanceData.isAreaLight = 1;

    if (!modelOffset.count(model)) {
        instanceData.indexOffset  = indicesArray.size();
        instanceData.vertexOffset = verticesArray.size();

        std::copy(model->GetIndices().begin(), model->GetIndices().end(), std::back_inserter(indicesArray));
        std::copy(model->GetVertices().begin(), model->GetVertices().end(), std::back_inserter(verticesArray));
        modelOffset.emplace(model, std::make_pair(instanceData.indexOffset, instanceData.vertexOffset));
    }
    else {
        instanceData.indexOffset  = modelOffset[model].first;
        instanceData.vertexOffset = modelOffset[model].second;
    }
}

void GPUInstance::Update()
{
    instanceStatus = Status::None;

    if (mesh->GetEntity()->GetComponent<Transform>()->GetUpdateStatus() == Transform::UpdateStatus::Transformation) {
        instanceStatus               = Status::MatrixChanged;
        instanceData.modelMatrix     = mesh->GetEntity()->GetComponent<Transform>()->GetWorldMatrix();
        instanceData.prevModelMatrix = mesh->GetEntity()->GetComponent<Transform>()->GetPrevWorldMatrix();
    }

    if (mesh->GetUpdateStatus() == Mesh::UpdateStatus::MeshAlter) {
        // Need to be optimal
        if (mesh->GetModel() != model) {
            model                     = mesh->GetModel();
            instanceData.indexOffset  = indicesArray.size();
            instanceData.vertexOffset = verticesArray.size();

            std::copy(model->GetIndices().begin(), model->GetIndices().end(), indicesArray.end());
            std::copy(model->GetVertices().begin(), model->GetVertices().end(), verticesArray.end());
            modelOffset.emplace(model, std::make_pair(instanceData.indexOffset, instanceData.vertexOffset));
        }
        instanceStatus = Status::ModelChanged;

        // MaterialId Update if material add? or delete
    }
}

bool GPUInstance::HasFlag(const std::string& flagName) const
{
    if (const auto& entity = mesh->GetEntity()) {
        if (entity->HasFlag(flagName)) return true;
    }
    return false;
}

Entity* GPUInstance::ParentsHasFlagEntity(const std::string& flagName) const
{
    if (auto* entity = mesh->GetEntity()) {
        while (entity != nullptr) {
            if (entity->HasFlag(flagName)) return entity;
            entity = entity->GetParent();
        }
    }
    return nullptr;
}
}   // namespace MapleLeaf