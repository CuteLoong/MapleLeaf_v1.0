#include "Mesh.hpp"

#include "Entity.hpp"
#include "Scenes.hpp"
#include <memory>

namespace MapleLeaf {
Mesh::Mesh(std::shared_ptr<Model> model, std::shared_ptr<Material> material, std::optional<uint32_t> instanceId)
    : model(model)
    , material(material)
    , instanceId(instanceId.value_or(0xFFFFFFFF))
{
    updateStatus = UpdateStatus::MeshAlter;
}


void Mesh::Start()
{
    if (material) material->CreatePipeline(GetVertexInput());
}

void Mesh::Update()
{
    updateStatus = UpdateStatus::None;

    if (material) {
        auto transform = GetEntity()->GetComponent<Transform>();
        material->PushUniforms(uniformObject, transform);
    }
}

bool Mesh::CmdRender(const CommandBuffer& commandBuffer, UniformHandler& uniformScene, const Pipeline::Stage& pipelineStage)
{
    if (!model || !material) return false;

    // TODO: check mesh in view

    // Check if we are in the correct pipeline stage.
    auto materialPipeline = material->GetPipelineMaterial();
    if (!materialPipeline || materialPipeline->GetStage() != pipelineStage) return false;

    // Binds the material pipeline.
    if (!materialPipeline->BindPipeline(commandBuffer)) return false;

    const auto& pipeline = *materialPipeline->GetPipeline();

    // Updates descriptors.
    descriptorSet.Push("uniformScene", uniformScene);
    descriptorSet.Push("UniformObject", uniformObject);

    material->PushDescriptors(descriptorSet);

    if (!descriptorSet.Update(pipeline)) return false;

    // Draws the object.
    descriptorSet.BindDescriptor(commandBuffer, pipeline);
    return model->CmdRender(commandBuffer);
}

void Mesh::SetMaterial(std::shared_ptr<Material>& material)
{
    this->material = material;
    this->material->CreatePipeline(GetVertexInput());
}

bool Mesh::operator<(const Mesh& rhs) const
{
    auto camera = Scenes::Get()->GetScene()->GetCamera();

    auto transform0 = GetEntity()->GetComponent<Transform>();
    auto transform1 = rhs.GetEntity()->GetComponent<Transform>();

    auto thisDistance  = glm::distance(camera->GetPosition(), transform0->GetPosition());
    auto otherDistance = glm::distance(camera->GetPosition(), transform1->GetPosition());

    return thisDistance > otherDistance;
}

bool Mesh::operator>(const Mesh& rhs) const
{
    return !operator<(rhs);
}
}   // namespace MapleLeaf