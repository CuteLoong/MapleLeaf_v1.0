#pragma once

#include "Color.hpp"
#include "Image2d.hpp"
#include "Material.hpp"

namespace MapleLeaf {
class DefaultMaterial : public Material::Registrar<DefaultMaterial>
{
    inline static const bool Registered = Register("default");

public:
    explicit DefaultMaterial(const Color& baseDiffuse = Color::White, std::shared_ptr<Image2d> imageDiffuse = nullptr,
                             std::shared_ptr<Image2d> imageEmissive = nullptr, float metallic = 0.0f, float roughness = 0.0f,
                             std::shared_ptr<Image2d> imageMaterial = nullptr, std::shared_ptr<Image2d> imageNormal = nullptr,
                             bool castsShadows = true, bool ignoreLighting = false, bool ignoreFog = false);

    const Color& GetBaseDiffuse() const { return baseDiffuse; }
    void         SetBaseDiffuse(const Color& baseDiffuse) { this->baseDiffuse = baseDiffuse; }

    const std::shared_ptr<Image2d>& GetImageDiffuse() const { return imageDiffuse; }
    void                            SetImageDiffuse(const std::shared_ptr<Image2d>& imageDiffuse) { this->imageDiffuse = imageDiffuse; }

    const Color& GetBaseEmissive() const { return baseEmissive; }
    void         SetBaseEmissive(const Color& baseEmissive) { this->baseEmissive = baseEmissive; }

    const std::shared_ptr<Image2d>& GetImageEmissive() const { return imageEmissive; }
    void                            SetImageEmissive(const std::shared_ptr<Image2d>& imageEmissive) { this->imageEmissive = imageEmissive; }

    float GetEmissiveIntensity() const { return emissiveIntensity; }
    void  SetEmissiveIntensity(float emissiveIntensity) { this->emissiveIntensity = emissiveIntensity; }

    float GetSpecularFactor() const { return specularFactor; }
    void  SetSpecularFactor(float specularFactor) { this->specularFactor = specularFactor; }

    float GetMetallic() const { return metallic; }
    void  SetMetallic(float metallic) { this->metallic = metallic; }

    float GetRoughness() const { return roughness; }
    void  SetRoughness(float roughness) { this->roughness = roughness; }

    const std::shared_ptr<Image2d>& GetImageMaterial() const { return imageMaterial; }
    void                            SetImageMaterial(const std::shared_ptr<Image2d>& imageMaterial) { this->imageMaterial = imageMaterial; }

    const std::shared_ptr<Image2d>& GetImageNormal() const { return imageNormal; }
    void                            SetImageNormal(const std::shared_ptr<Image2d>& imageNormal) { this->imageNormal = imageNormal; }

    bool IsCastsShadows() const { return castsShadows; }
    void SetCastsShadows(bool castsShadows) { this->castsShadows = castsShadows; }

    bool IsIgnoringLighting() const { return ignoreLighting; }
    void SetIgnoreLighting(bool ignoreLighting) { this->ignoreLighting = ignoreLighting; }

    bool IsIgnoringFog() const { return ignoreFog; }
    void SetIgnoreFog(bool ignoreFog) { this->ignoreFog = ignoreFog; }

private:
    std::vector<Shader::Define> GetDefines() const;

    Color                    baseDiffuse;
    Color                    baseEmissive;
    std::shared_ptr<Image2d> imageDiffuse;
    std::shared_ptr<Image2d> imageEmissive;

    float emissiveIntensity;

    float                    specularFactor;
    float                    metallic;
    float                    roughness;
    std::shared_ptr<Image2d> imageMaterial;
    std::shared_ptr<Image2d> imageNormal;

    bool castsShadows;
    bool ignoreLighting;
    bool ignoreFog;
};
}   // namespace MapleLeaf