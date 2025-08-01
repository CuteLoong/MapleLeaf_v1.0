#include "LightSystem.hpp"

#include "Light.hpp"
#include "Scenes.hpp"
#include "Transform.hpp"

namespace MapleLeaf {

LightSystem::LightSystem()
    : LTCTexture1(LoadLTCTexture1())
    , LTCTexture2(LoadLTCTexture2())
    , blueNoise(LoadBlueNoise())
    , pointLights(1, PointLight())
    , directionalLights(1, DirectionalLight())
    , areaLights(1, AreaLight())
{
    storagePointLights = std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(pointLights.size() * sizeof(PointLight)), pointLights.data());
    storageDirectionalLights =
        std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(directionalLights.size() * sizeof(DirectionalLight)), directionalLights.data());
    storageAreaLights = std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(areaLights.size() * sizeof(AreaLight)), areaLights.data());
}

void LightSystem::Update()
{
    auto sceneLights             = Scenes::Get()->GetScene()->GetComponents<Light>();
    bool updatePointLights       = false;
    bool updateDirectionalLights = false;
    bool updateAreaLights        = false;

    for (const auto& light : sceneLights) {
        if (light->GetEntity()->GetComponent<Transform>()->GetUpdateStatus() != Transform::UpdateStatus::Transformation) continue;
        if (light->type == LightType::Directional) {
            DirectionalLight directionalLight = {};
            directionalLight.color            = light->GetColor();
            directionalLight.direction        = light->GetDirection();
            directionalLights.push_back(directionalLight);
            updateDirectionalLights = true;
        }
        else if (light->type == LightType::Point) {
            PointLight pointLight = {};
            pointLight.color      = light->GetColor();
            if (auto transform = light->GetEntity()->GetComponent<Transform>()) {
                pointLight.position = transform->GetPosition();
            }
            pointLight.attenuation = light->GetAttenuation();
            pointLights.push_back(pointLight);
            updatePointLights = true;
        }
        else if (light->type == LightType::Area) {
            AreaLight areatLight = {};
            areatLight.color     = light->GetColor();
            if (auto transform = light->GetEntity()->GetComponent<Transform>()) {
                for (int i = 0; i < light->GetPoints().size(); i++) {
                    areatLight.points[i] = transform->GetWorldMatrix() * glm::vec4(light->GetPoints()[i], 1.0f);
                }
            }
            areatLight.twoSided  = light->GetTwoSide();
            areatLight.intensity = light->GetIntensity();
            areaLights.push_back(areatLight);
            updateAreaLights = true;
        }
    }
    if (updatePointLights) {
        if (storagePointLights == nullptr || storagePointLights->GetSize() < static_cast<VkDeviceSize>(pointLights.size() * sizeof(PointLight)))
            storagePointLights =
                std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(pointLights.size() * sizeof(PointLight)), pointLights.data());
        else
            storagePointLights->Update(pointLights.data(), static_cast<VkDeviceSize>(pointLights.size() * sizeof(PointLight)));
    }

    if (updateDirectionalLights) {
        if (storageDirectionalLights == nullptr ||
            storageDirectionalLights->GetSize() < static_cast<VkDeviceSize>(directionalLights.size() * sizeof(DirectionalLight)))
            storageDirectionalLights = std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(directionalLights.size() * sizeof(DirectionalLight)),
                                                                       directionalLights.data());
        else
            storageDirectionalLights->Update(directionalLights.data(),
                                             static_cast<VkDeviceSize>(directionalLights.size() * sizeof(DirectionalLight)));
    }

    if (updateAreaLights) {
        if (storageAreaLights == nullptr || storageAreaLights->GetSize() < static_cast<VkDeviceSize>(areaLights.size() * sizeof(AreaLight)))
            storageAreaLights = std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(areaLights.size() * sizeof(AreaLight)), areaLights.data());
        else
            storageAreaLights->Update(areaLights.data(), static_cast<VkDeviceSize>(areaLights.size() * sizeof(AreaLight)));
    }

    // if (updateStorage || storagePointLights == nullptr || storageDirectionalLights == nullptr || storageAreaLights == nullptr) {
    //     storagePointLights = std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(pointLights.size() * sizeof(PointLight)),
    //     pointLights.data()); storageDirectionalLights =
    //         std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(directionalLights.size() * sizeof(DirectionalLight)),
    //         directionalLights.data());
    //     storageAreaLights = std::make_unique<StorageBuffer>(static_cast<VkDeviceSize>(areaLights.size() * sizeof(AreaLight)), areaLights.data());
    // }
}

std::shared_ptr<Image2d> LightSystem::LoadLTCTexture1()
{
    auto LTCImage1 = Image2d::Create("Precomputed/LTC_Inverse_M.exr", VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, false, false);

    return std::move(LTCImage1);
}

std::shared_ptr<Image2d> LightSystem::LoadLTCTexture2()
{
    auto LTCImage2 = Image2d::Create("Precomputed/LTC_Fresnel_Scale.exr", VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, false, false);

    return std::move(LTCImage2);
}

std::shared_ptr<Image2d> LightSystem::LoadBlueNoise()
{
    auto blueNoise = Image2d::Create("NoiseImage/BlueNoise.tga", VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, false, false);

    return std::move(blueNoise);
}
}   // namespace MapleLeaf