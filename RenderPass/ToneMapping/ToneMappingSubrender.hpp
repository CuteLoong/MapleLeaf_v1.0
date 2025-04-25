#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineGraphics.hpp"
#include "Subrender.hpp"
#include "UniformHandler.hpp"

namespace MapleLeaf {
class ToneMappingSubrender : public Subrender
{
public:
    enum class Type
    {
        Linear           = 0,
        Reinhard         = 1,
        ReinhardModified = 2,
        HejiHableAlu     = 3,
        HableUc2         = 4,
        Aces             = 5,
    };
    struct ToneMappingInfo
    {
        float exposure;
        float gamma;
        float whiteMaxLuminance;
        float whiteScale;
        Type  type;

        ToneMappingInfo(float exposure = 1.0f, float gamma = 2.2f, float whiteMaxLuminance = 1.0f, float whiteScale = 11.2f, Type type = Type::Aces)
            : type(type)
            , exposure(exposure)
            , gamma(gamma)
            , whiteMaxLuminance(whiteMaxLuminance)
            , whiteScale(whiteScale)
        {}
    };
    explicit ToneMappingSubrender(const Pipeline::Stage& pipelineStage, ToneMappingInfo toneMappingInfo = ToneMappingInfo());

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    void RegisterImGui() override;

private:
    PipelineGraphics pipeline;

    DescriptorsHandler descriptorSet;
    UniformHandler     uniformToneMapping;

    ToneMappingInfo toneMappingInfo;
};
}   // namespace MapleLeaf