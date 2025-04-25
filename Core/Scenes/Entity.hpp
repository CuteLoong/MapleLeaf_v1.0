#pragma once

#include "Component.hpp"
#include "NonCopyable.hpp"
#include <unordered_map>

namespace MapleLeaf {
class Entity final : NonCopyable
{
public:
    Entity() = default;

    void Update();

    const std::string& GetName() const { return name; }
    void               SetName(const std::string& name) { this->name = name; }

    bool IsRemoved() const { return removed; }
    void SetRemoved(bool removed) { this->removed = removed; }

    const std::vector<std::unique_ptr<Component>>& GetComponents() const { return components; }
    uint32_t                                       GetComponentCount() const { return static_cast<uint32_t>(components.size()); }

    /**
     * Gets a component by type.
     * @tparam T The component type to find.
     * @param allowDisabled If disabled components will be returned.
     * @return The found component.
     */
    template<typename T>
    T* GetComponent(bool allowDisabled = false) const
    {
        T* alternative = nullptr;

        for (const auto& component : components) {
            auto casted = dynamic_cast<T*>(component.get());

            if (casted) {
                if (allowDisabled && !component->IsEnabled()) {
                    alternative = casted;
                    continue;
                }

                return casted;
            }
        }

        return alternative;
    }

    /**
     * Gets components by type.
     * @tparam T The component type to find.
     * @param allowDisabled If disabled components will be returned.
     * @return The components.
     */
    template<typename T>
    std::vector<T*> GetComponents(bool allowDisabled = false) const
    {
        std::vector<T*> components;

        for (const auto& component : this->components) {
            auto casted = dynamic_cast<T*>(component.get());

            if (casted) {
                if (allowDisabled && !component->IsEnabled()) {
                    components.emplace_back(casted);
                    continue;
                }

                components.emplace_back(casted);
            }
        }

        return components;
    }

    /**
     * Adds a component to this entity.
     * @param component The component to add.
     * @return The added component.
     */
    Component* AddComponent(std::unique_ptr<Component>&& component);

    /**
     * Creates a component by type to be added this entity.
     * @tparam T The type of component to add.
     * @tparam Args The argument types/
     * @param args The type constructor arguments.
     * @return The added component.
     */
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        return dynamic_cast<T*>(AddComponent(std::make_unique<T>(std::forward<Args>(args)...)));
    }

    /**
     * Removes a component from this entity.
     * @param component The component to remove.
     */
    void RemoveComponent(Component* component);

    /**
     * Removes a component from this entity.
     * @param name The name of the component to remove.
     */
    void RemoveComponent(const std::string& name);

    /**
     * Removes a component by type from this entity.
     * @tparam T The type of component to remove.
     */
    template<typename T>
    void RemoveComponent()
    {
        for (auto it = components.begin(); it != components.end(); ++it) {
            auto casted = dynamic_cast<T*>((*it).get());

            if (casted) {
                (*it)->SetEntity(nullptr);
                components.erase(it);
            }
        }
    }

    /**
     * @brief Set the parent of this entity.
     * @param parent The parent entity.
     */
    void SetParent(Entity* parent);

    /**
     * @brief Check if this entity has a parent with the name.
     * @param name The name of the parent entity.
     */
    bool HasParent(const std::string& name) const;

    /**
     * @brief Get the parent of this entity.
     * @return The parent entity.
     */
    Entity* GetParent() const { return parent; }

    /**
     * @brief Get the children of this entity.
     * @return The children entities.
     */
    const std::vector<Entity*>& GetChildren() const { return children; }

    /**
     * @brief Set a flag on this entity.
     * @param flagName The name of the flag.
     * @param value The value to set (true or false).
     */
    void SetFlag(const std::string& flagName, bool value) { flags[flagName] = value; }

    /**
     * @brief Check if a flag exists on this entity.
     * @param flagName The name of the flag.
     * @return Whether the flag exists.
     */
    bool HasFlag(const std::string& flagName) const { return flags.find(flagName) != flags.end(); }

    /**
     * @brief Get the value of a flag.
     * @param flagName The name of the flag.
     * @param defaultValue The default value if the flag doesn't exist.
     * @return The flag value, or defaultValue if the flag doesn't exist.
     */
    bool GetFlag(const std::string& flagName, bool defaultValue = false) const
    {
        auto it = flags.find(flagName);
        return (it != flags.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Remove a flag from this entity.
     * @param flagName The name of the flag to remove.
     */
    void RemoveFlag(const std::string& flagName) { flags.erase(flagName); }

    /**
     * @brief Clear all flags from this entity.
     */
    void ClearFlags() { flags.clear(); }

    /**
     * @brief Get all flags as a const reference.
     * @return The flags map.
     */
    const std::unordered_map<std::string, bool>& GetFlags() const { return flags; }

private:
    /**
     * @brief Remove the child entity from this entity.
     * @param child The child entity to remove.
     */
    void RemoveChild(Entity* child);

    /**
     * @brief Add a child entity to this entity.
     * @param child The child entity to add.
     */
    void AddChild(Entity* child) { children.emplace_back(child); }

    std::string                             name;
    bool                                    removed = false;
    std::vector<std::unique_ptr<Component>> components;

    Entity*              parent = nullptr;
    std::vector<Entity*> children;
    
    std::unordered_map<std::string, bool> flags;  // 存储实体的标志列表
};
}   // namespace MapleLeaf