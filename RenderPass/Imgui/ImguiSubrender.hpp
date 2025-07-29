#pragma once

#include "Subrender.hpp"

namespace MapleLeaf {
class ImguiSubrender : public Subrender
{
public:
    explicit ImguiSubrender(const Pipeline::Stage& pipelineStage);

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    void RegisterImGui() override;

private:
};
}   // namespace MapleLeaf