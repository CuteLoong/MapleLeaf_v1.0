#include "Entity.hpp"

namespace MapleLeaf {
void Entity::Update()
{
    for (auto it = components.begin(); it != components.end(); ++it) {
        if ((*it)->IsRemoved()) {
            it = components.erase(it);
            continue;
        }

        if ((*it)->GetEntity() != this) (*it)->SetEntity(this);

        if ((*it)->IsEnabled()) {
            if (!(*it)->started) {
                (*it)->Start();
                (*it)->started = true;
            }

            (*it)->Update();
        }
    }
}

Component* Entity::AddComponent(std::unique_ptr<Component>&& component)
{
    if (!component) return nullptr;

    component->SetEntity(this);
    return components.emplace_back(std::move(component)).get();
}

void Entity::RemoveComponent(Component* component)
{
    components.erase(std::remove_if(components.begin(), components.end(), [component](const auto& c) { return c.get() == component; }),
                     components.end());
}

void Entity::RemoveComponent(const std::string& name)
{
    components.erase(std::remove_if(components.begin(), components.end(), [name](const auto& c) { return name == c->GetTypeName(); }),
                     components.end());
}

bool Entity::HasParent(const std::string& name) const
{
    Entity* parent = GetParent();
    while (parent) {
        if (parent->GetName() == name) return true;
        parent = parent->GetParent();
    }
    return false;
}

bool Entity::HasParentContainFlag(const std::string& flagName) const
{
    Entity* parent = GetParent();
    while (parent) {
        if (parent->GetFlag(flagName)) return true;
        parent = parent->GetParent();
    }
    return false;
}

void Entity::SetParent(Entity* parent)
{
    if (this->parent) this->parent->RemoveChild(this);
    this->parent = parent;
    if (this->parent) parent->AddChild(this);
}


void Entity::RemoveChild(Entity* child)
{
    children.erase(std::remove(children.begin(), children.end(), child), children.end());
}
}   // namespace MapleLeaf