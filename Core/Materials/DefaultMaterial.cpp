#include "DefaultMaterial.hpp"
#include <string>

namespace MapleLeaf {
DefaultMaterial::DefaultMaterial(const Color& baseDiffuse, std::shared_ptr<Image2d> imageDiffuse, std::shared_ptr<Image2d> imageEmissive,
                                 float metallic, float roughness, std::shared_ptr<Image2d> imageMaterial, std::shared_ptr<Image2d> imageNormal,
                                 bool castsShadows, bool ignoreLighting, bool ignoreFog)
    : baseDiffuse(baseDiffuse)
    , imageDiffuse(std::move(imageDiffuse))
    , imageEmissive(std::move(imageEmissive))
    , metallic(metallic)
    , roughness(roughness)
    , imageMaterial(std::move(imageMaterial))
    , imageNormal(std::move(imageNormal))
    , castsShadows(castsShadows)
    , ignoreLighting(ignoreLighting)
    , ignoreFog(ignoreFog)
{}

std::vector<Shader::Define> DefaultMaterial::GetDefines() const
{
    return {
        {"DIFFUSE_MAPPING", std::to_string(static_cast<uint32_t>((imageDiffuse != nullptr)))},
        {"MATERIAL_MAPPING", std::to_string(static_cast<uint32_t>(imageMaterial != nullptr))},
        {"NORMAL_MAPPING", std::to_string(static_cast<uint32_t>(imageNormal != nullptr))},
    };
}
}   // namespace MapleLeaf