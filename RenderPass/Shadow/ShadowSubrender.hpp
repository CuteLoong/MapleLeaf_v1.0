#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineGraphics.hpp"
#include "Subrender.hpp"
#include "UniformHandler.hpp"

namespace MapleLeaf {
class ShadowSubrender : public Subrender
{
public:
    explicit ShadowSubrender(const Pipeline::Stage& stage);

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;

    void RegisterImGui() override;

private:
    PipelineGraphics   pipeline;
    DescriptorsHandler descriptorSet;
    UniformHandler     uniformObject;
};
}   // namespace MapleLeaf
