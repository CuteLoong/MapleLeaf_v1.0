#include "ResolvedSubrender.hpp"
#include "Graphics.hpp"

namespace MapleLeaf {
ResolvedSubrender::ResolvedSubrender(const Pipeline::Stage& pipelineStage)
    : Subrender(pipelineStage)
    , pipeline(pipelineStage, {"Shader/Resolved/Resolved.vert", "Shader/Resolved/Resolved.frag"}, {}, {}, PipelineGraphics::Mode::Polygon,
               PipelineGraphics::Depth::None)
    , descriptorSet(pipeline)
{}

void ResolvedSubrender::RegisterImGui() {}

void ResolvedSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void ResolvedSubrender::Render(const CommandBuffer& commandBuffer)
{
    descriptorSet.Push("Lighting", Graphics::Get()->GetAttachment("lighting"));

    if (!descriptorSet.Update(pipeline)) return;
    pipeline.BindPipeline(commandBuffer);
    descriptorSet.BindDescriptor(commandBuffer, pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void ResolvedSubrender::PostRender(const CommandBuffer& commandBuffer) {}
}   // namespace MapleLeaf
