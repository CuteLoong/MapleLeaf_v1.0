#pragma once

#include "ASScene.hpp"
#include "Camera.hpp"
#include "DerivedSceneHolder.hpp"
#include "EntityHolder.hpp"
#include "SystemHolder.hpp"

#include "config.h"

namespace MapleLeaf {
class Scene
{
    friend class Scenes;

public:
    Scene();
    virtual ~Scene()
    {
        systems.Clear();
        entities.Clear();
    }

    virtual void Start() = 0;
    virtual void Update();

    template<typename T>
    bool HasSystem() const
    {
        return systems.Has<T>();
    }

    template<typename T>
    T* GetSystem() const
    {
        return systems.Get<T>();
    }

    template<typename T, typename... Args>
    void AddSystem(Args&&... args)
    {
        systems.Add<T>(std::make_unique<T>(std::forward<Args>(args)...));
    }

    template<typename T>
    void RemoveSystem()
    {
        systems.Remove<T>();
    }

    void ClearSystems() { systems.Clear(); }

    Entity* GetEntity(const std::string& name) const { return entities.GetEntity(name); }

    Entity* CreateEntity() { return entities.CreateEntity(); }

    std::vector<Entity*> QueryAllEntities() { return entities.QueryAll(); }

    template<typename T, typename... Args>
    void AddDerivedScene(Args&&... args)
    {
        derivedScenes.Add<T>(std::make_unique<T>(std::forward<Args>(args)...));
    }

    template<typename T>
    T* GetComponent(bool allowDisabled = false)
    {
        return entities.GetComponent<T>(allowDisabled);
    }

    template<typename T>
    std::vector<T*> GetComponents(bool allowDisabled = false)
    {
        return entities.GetComponents<T>(allowDisabled);
    }

    template<typename T>
    T* GetDerivedScene() const
    {
        return derivedScenes.Get<T>();
    }

    template<typename T>
    bool HasDerivedScene() const
    {
        return derivedScenes.Has<T>();
    }

    void ClearEntities() { entities.Clear(); }

    Camera* GetCamera() const { return camera; }
    void    SetCamera(Camera* camera) { this->camera = camera; }

    bool IsPaused();
    void SetPaused(bool paused) { this->paused = paused; }

    bool IsStarted() const { return started; }

    const glm::vec3& GetMinExtents() const { return minExtents; }
    const glm::vec3& GetMaxExtents() const { return maxExtents; }
    void             SetExtents(const glm::vec3& maxExtent, const glm::vec3& minExtent, const glm::mat4& transfrom);

private:
    bool started = false;
    bool paused  = false;

    SystemHolder       systems;
    EntityHolder       entities;
    DerivedSceneHolder derivedScenes;
    Camera*            camera;

    glm::vec3 minExtents = glm::vec3(std::numeric_limits<float>::infinity());
    glm::vec3 maxExtents = glm::vec3(-std::numeric_limits<float>::infinity());
};
}   // namespace MapleLeaf