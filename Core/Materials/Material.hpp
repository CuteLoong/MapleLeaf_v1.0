#pragma once

#include "DescriptorHandler.hpp"
#include "MaterialPipeline.hpp"
#include "Resource.hpp"
#include "StreamFactory.hpp"
#include "Transform.hpp"
#include "UniformHandler.hpp"
#include "glm/fwd.hpp"

namespace MapleLeaf {
class Material : public StreamFactory<Material>, public Resource
{
public:
    enum class TextureSlot
    {
        BaseColor,
        Emissive,
        Material,   // metalic roughness's image
        Normal,
    };
    std::type_index GetTypeIndex() const override { return typeid(Material); }

    virtual ~Material() = default;

    void SetEmissive() { emissive = true; }
    bool IsEmissive() const { return emissive; }

protected:
    bool emissive = false;
};

template class TypeInfo<Material>;
}   // namespace MapleLeaf