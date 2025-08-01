#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineRayTracing.hpp"
#include "Subrender.hpp"
#include "UniformHandler.hpp"

namespace MapleLeaf {
class RayTracingSubrender : public Subrender
{
public:
    explicit RayTracingSubrender(const Pipeline::Stage& pipelineStage);

    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    void RegisterImGui() override;

private:
    PipelineRayTracing pipelineRayTracing;

    UniformHandler     uniformGeometry;
    UniformHandler     uniformScene;
    UniformHandler     uniformFrameData;
    UniformHandler     uniformCamera;
    DescriptorsHandler descriptorSet;

    float wallroughness = 0.5f;
};
}   // namespace MapleLeaf