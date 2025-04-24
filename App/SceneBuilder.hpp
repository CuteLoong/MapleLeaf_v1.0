#pragma once

#include "AnimationController.hpp"
#include "AssimpImporter.hpp"
#include "Camera.hpp"
#include "DefaultBuilder.hpp"
#include "DefaultMaterial.hpp"
#include "GPUScene.hpp"
#include "LightSystem.hpp"
#include "Resources.hpp"
#include "Scene.hpp"
#include "ShadowRender.hpp"
#include "ShadowSystem.hpp"
#include "SkyboxSystem.hpp"
#include "Transform.hpp"

namespace MapleLeaf {
class SceneBuilder : public Scene
{
public:
    SceneBuilder(const std::filesystem::path& path)
        : Scene()
        , path(path)
    {
        AddSystem<ShadowSystem>();
        AddSystem<LightSystem>();
        AddSystem<SkyboxSystem>();

        AddDerivedScene<GPUScene>();
        AddDerivedScene<ASScene>();
    }

    void Start() override
    {
        Builder                         builder;
        AssimpImporter<DefaultMaterial> assimpImporter;

        assimpImporter.Import(path, builder);

        auto     shadows       = GetSystem<ShadowSystem>();
        uint32_t instanceCount = 0;

        for (uint32_t index = 0; index < builder.sceneGraph.size(); index++) {
            const auto& node = builder.sceneGraph[index];

            auto entity = CreateEntity();
            entity->SetName(node.name);

            if (node.parent.isValid()) {
                const auto& parent = builder.sceneGraph[node.parent.get()];
                entity->SetParent(GetEntity(parent.name));
            }

            std::unique_ptr<Transform> transform;
            transform.reset(node.transform);
            entity->AddComponent(std::move(transform));

            if (builder.animations.find(index) != builder.animations.end()) entity->AddComponent<AnimationController>(builder.animations[index]);

            if (!node.meshes.empty()) {
                for (int i = 0; i < node.meshes.size(); i++) {
                    this->SetExtents(builder.meshes[node.meshes[i]]->GetModel()->GetMaxExtents(),
                                     builder.meshes[node.meshes[i]]->GetModel()->GetMinExtents(),
                                     node.transform->GetWorldMatrix());
                    Resources::Get()->Add(builder.meshes[node.meshes[i]]->GetModel());

                    entity->AddComponent<Mesh>(
                        builder.meshes[node.meshes[i]]->GetModel(), builder.meshes[node.meshes[i]]->GetMaterial(), instanceCount);
                    instanceCount++;
                    if (shadows) entity->AddComponent<ShadowRender>();
                }
            }
        }

        if (builder.cameras.empty()) Log::Error("No camera can't rendering!");

        for (auto& camera : builder.cameras) {
            auto entity = GetEntity(camera->GetName());
            entity->AddComponent(std::move(camera));
        }

        SetCamera(GetComponent<Camera>());

        for (auto& light : builder.lights) {
            auto entity = GetEntity(light->GetName());
            if (entity) {
                auto transform = entity->GetComponent<Transform>();

                glm::vec4 realDirection = transform->GetWorldMatrix() * glm::vec4(light->GetDirection(), 0.0f);

                light->SetPosition(transform->GetPosition());
                light->SetDirection(glm::vec3(realDirection));

                if (shadows && (light->type == LightType::Directional)) shadows->SetLightDirection(light->GetDirection());

                entity->AddComponent(std::move(light));
            }
        }
    }

    void Update() override { Scene::Update(); }

private:
    std::filesystem::path path;
};
}   // namespace MapleLeaf