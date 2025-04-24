#pragma once

#include "PipelineGraphics.hpp"
#include "Subrender.hpp"

namespace MapleLeaf {
class DefaultSubrender : public Subrender
{
public:
    explicit DefaultSubrender(const Pipeline::Stage& pipelineStage);

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    void RegisterImGui() override;

private:
    PipelineGraphics pipeline;
};
}   // namespace MapleLeaf