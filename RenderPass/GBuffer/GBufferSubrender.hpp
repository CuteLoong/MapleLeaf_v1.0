#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineCompute.hpp"
#include "PipelineGraphics.hpp"
#include "Subrender.hpp"
#include "UniformHandler.hpp"

namespace MapleLeaf {
class GBufferSubrender : public Subrender
{
public:
    explicit GBufferSubrender(const Pipeline::Stage& stage);
    ~GBufferSubrender() override = default;

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;

    void RegisterImGui() override;

private:
    PipelineGraphics pipeline;
    PipelineCompute  compute;

    DescriptorsHandler descriptorSetCompute;
    DescriptorsHandler descriptorSetGraphics;

    PushHandler    pushHandler;
    UniformHandler uniformCamera;
};
}   // namespace MapleLeaf
