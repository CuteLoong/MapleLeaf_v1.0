#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineGraphics.hpp"
#include "Subrender.hpp"
#include "UniformHandler.hpp"

namespace MapleLeaf {
class SkyboxMappingSubrender : public Subrender
{
public:
    explicit SkyboxMappingSubrender(const Pipeline::Stage& pipelineStage);

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    void RegisterImGui() override;

private:
    PipelineGraphics pipelineGraphics;

    std::vector<UniformHandler> uniformSkybox;
    UniformHandler              uniformParams;

    std::vector<DescriptorsHandler> descriptorSet;
    int                             maxMipLevel;
};
}   // namespace MapleLeaf