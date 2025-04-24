#pragma once

#include "Color.hpp"
#include "Future.hpp"
#include "Image2d.hpp"
#include "StorageBuffer.hpp"
#include "System.hpp"

#include <array>

namespace MapleLeaf {
class LightSystem : public System
{
public:
    struct PointLight
    {
        Color color                       = Color::White;
        alignas(16) glm::vec3 position    = glm::vec3(0.0f);
        alignas(16) glm::vec3 attenuation = glm::vec3(0.0f);
    };

    struct DirectionalLight
    {
        Color color                     = Color::White;
        alignas(16) glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
    };

    struct AreaLight
    {
        Color color                                 = Color::White;
        alignas(16) std::array<glm::vec4, 4> points = {glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(0.0f)};
        uint32_t twoSided                           = 0;
        float    intensity                          = 1.0f;
    };

    explicit LightSystem();

    void Update() override;

    uint32_t GetPointLightsCount() const { return static_cast<uint32_t>(pointLights.size()); }
    uint32_t GetDirectionalLightsCount() const { return static_cast<uint32_t>(directionalLights.size()); }
    uint32_t GetAreaLightsCount() const { return static_cast<uint32_t>(areaLights.size()); }

    const StorageBuffer* GetStoragePointLights() const { return storagePointLights.get(); }
    const StorageBuffer* GetStorageDirectionalLights() const { return storageDirectionalLights.get(); }
    const StorageBuffer* GetStorageAreaLights() const { return storageAreaLights.get(); }

    const Image2d* GetLTCTexture1() { return (*LTCTexture1).get(); }
    const Image2d* GetLTCTexture2() { return (*LTCTexture2).get(); }
    const Image2d* GetBlueNoise() { return (*blueNoise).get(); }

private:
    std::vector<PointLight>       pointLights;
    std::vector<DirectionalLight> directionalLights;
    std::vector<AreaLight>        areaLights;

    std::unique_ptr<StorageBuffer> storagePointLights;
    std::unique_ptr<StorageBuffer> storageDirectionalLights;
    std::unique_ptr<StorageBuffer> storageAreaLights;

    Future<std::shared_ptr<Image2d>> LTCTexture1;
    Future<std::shared_ptr<Image2d>> LTCTexture2;
    Future<std::shared_ptr<Image2d>> blueNoise;

    static std::shared_ptr<Image2d> LoadLTCTexture1();
    static std::shared_ptr<Image2d> LoadLTCTexture2();
    static std::shared_ptr<Image2d> LoadBlueNoise();
};

}   // namespace MapleLeaf