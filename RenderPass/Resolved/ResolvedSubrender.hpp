#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineGraphics.hpp"
#include "Subrender.hpp"

namespace MapleLeaf {
class ResolvedSubrender : public Subrender
{
public:
    explicit ResolvedSubrender(const Pipeline::Stage& pipelineStage);

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    void RegisterImGui() override;

private:
    PipelineGraphics pipeline;

    DescriptorsHandler descriptorSet;
};
}   // namespace MapleLeaf
