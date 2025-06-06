#pragma once

#include "Graphics.hpp"
#include "Resources.hpp"
#include "Scene.hpp"


namespace MapleLeaf {
class Scenes : public Module::Registrar<Scenes>
{
    inline static const bool Registered = Register(Stage::Normal, Requires<Graphics, Resources>());

public:
    Scenes();

    void Update() override;

    Scene* GetScene() const { return scene.get(); }
    void   ClearScene() { scene.reset(); }
    void   SetScene(std::unique_ptr<Scene>&& scene) { this->scene = std::move(scene); }

private:
    std::unique_ptr<Scene> scene;
};
}   // namespace MapleLeaf