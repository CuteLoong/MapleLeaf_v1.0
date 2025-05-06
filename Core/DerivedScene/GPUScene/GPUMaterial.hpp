#pragma once

#include "Material.hpp"

namespace MapleLeaf {
class GPUMaterial
{
    friend class GPUScene;

public:
    struct MaterialData
    {
        Color   baseColor;
        float   metalic;
        float   roughness;
        int32_t baseColorTex;
        int32_t normalTex;
        int32_t materialTex;
        int32_t padding1;
        int32_t padding2;
        int32_t padding3;
    };

    GPUMaterial() = default;
    explicit GPUMaterial(const std::shared_ptr<Material>& material);

    MaterialData GetMaterialData() const { return materialData; }

    static std::optional<uint32_t>               GetMaterialID(const std::shared_ptr<Material>& material);
    static std::vector<std::shared_ptr<Image2d>> GetImages() { return images; }

private:
    std::shared_ptr<Material> material;

    MaterialData materialData;

    static std::vector<std::shared_ptr<Material>> materialArray;
    static std::vector<std::shared_ptr<Image2d>>  images;
};
}   // namespace MapleLeaf