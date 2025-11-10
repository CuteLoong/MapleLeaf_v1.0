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


void Mesh::Start() {}

void Mesh::Update()
{
    updateStatus = UpdateStatus::None;
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