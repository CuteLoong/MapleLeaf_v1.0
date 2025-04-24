#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineGraphics.hpp"
#include "Subrender.hpp"
#include "UniformHandler.hpp"

namespace MapleLeaf {
class SkyboxSubrender : public Subrender
{
public:
    explicit SkyboxSubrender(const Pipeline::Stage& pipelineStage);

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    void RegisterImGui() override;

private:
    PipelineGraphics pipelineGraphics;

    UniformHandler uniformCamera;
    UniformHandler uniformSkybox;

    DescriptorsHandler descriptorSet;
};
}   // namespace MapleLeaf