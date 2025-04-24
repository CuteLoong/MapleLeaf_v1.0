#include "GPUMaterial.hpp"
#include "DefaultMaterial.hpp"

namespace MapleLeaf {
std::vector<std::shared_ptr<Material>> GPUMaterial::materialArray{};
std::vector<std::shared_ptr<Image2d>>  GPUMaterial::images{};

std::optional<uint32_t> GPUMaterial::GetMaterialID(const std::shared_ptr<Material>& material)
{
    if (const auto& it = std::find(materialArray.begin(), materialArray.end(), material); it != materialArray.end()) {
        return static_cast<uint32_t>(std::distance(materialArray.begin(), it));
    }
    return std::nullopt;
}

GPUMaterial::GPUMaterial(const std::shared_ptr<Material>& material)
    : material(material)
{
    if (const DefaultMaterial* defaultMaterial = dynamic_cast<const DefaultMaterial*>(material.get())) {
        materialData.baseColor    = defaultMaterial->GetBaseDiffuse();
        materialData.roughness    = defaultMaterial->GetRoughness();
        materialData.metalic      = defaultMaterial->GetMetallic();
        materialData.baseColorTex = -1;
        materialData.normalTex    = -1;
        materialData.materialTex  = -1;

        if (const auto& diffuseImage = defaultMaterial->GetImageDiffuse()) {
            if (const auto& it = std::find(images.begin(), images.end(), diffuseImage); it != images.end()) {
                materialData.baseColorTex = static_cast<int32_t>(std::distance(images.begin(), it));
            }
            else {
                materialData.baseColorTex = images.size();
                images.push_back(diffuseImage);
            }
        }

        if (const auto& normalImage = defaultMaterial->GetImageNormal()) {
            if (const auto& it = std::find(images.begin(), images.end(), normalImage); it != images.end()) {
                materialData.normalTex = static_cast<int32_t>(std::distance(images.begin(), it));
            }
            else {
                materialData.normalTex = images.size();
                images.push_back(normalImage);
            }
        }

        if (const auto& materialImage = defaultMaterial->GetImageMaterial()) {
            if (const auto& it = std::find(images.begin(), images.end(), materialImage); it != images.end()) {
                materialData.materialTex = static_cast<int32_t>(std::distance(images.begin(), it));
            }
            else {
                materialData.materialTex = images.size();
                images.push_back(materialImage);
            }
        }
    }

    materialArray.push_back(material);
}

}   // namespace MapleLeaf