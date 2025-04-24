#pragma once

#include "Future.hpp"
#include "Image2d.hpp"
#include "ImageCube.hpp"
#include "Skybox.hpp"
#include "System.hpp"

namespace MapleLeaf {
class SkyboxSystem : public System
{
public:
    explicit SkyboxSystem();

    void Update() override;

    void  SetLoaded(bool loaded) { this->loaded = loaded; }
    bool  IsLoaded() const { return loaded; }
    float GetGamma() const { return gamma; }
    float GetExposure() const { return exposure; }

    const Image2d*   GetBRDF() { return (*brdf).get(); }
    const ImageCube* GetIrradiance() { return (*irradiance).get(); }
    const ImageCube* GetPrefiltered() { return (*prefiltered).get(); }

    const ImageCube* GetSkybox() { return skybox->GetImageCube(); }
    const Image2d*   GetSkyboxEqMap() { return skybox->GetImage2d(); }
    const Transform* GetTransform() { return skybox->GetTransform(); }

    bool WaitMapping() const;
    void SetMapped(bool mapped) { skybox->SetMapped(mapped); }

    void ComputeSkybox();
    void RegisterCustomWindow();

private:
    bool      loaded;
    float     gamma;
    float     exposure;
    glm::vec3 rotation;

    std::unique_ptr<Skybox> skybox;

    Future<std::unique_ptr<Image2d>>   brdf;
    Future<std::unique_ptr<ImageCube>> irradiance;
    Future<std::unique_ptr<ImageCube>> prefiltered;

    static std::unique_ptr<Image2d>   ComputeBRDF(uint32_t size);
    static std::unique_ptr<ImageCube> ComputeIrradiance(const ImageCube* source, uint32_t size);
    static std::unique_ptr<ImageCube> ComputePrefiltered(const ImageCube* source, uint32_t size);
};

}   // namespace MapleLeaf