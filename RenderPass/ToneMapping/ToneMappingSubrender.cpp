#include "ToneMappingSubrender.hpp"
#include "Graphics.hpp"
#include "Imgui.hpp"

namespace MapleLeaf {
ToneMappingSubrender::ToneMappingSubrender(const Pipeline::Stage& pipelineStage, ToneMappingInfo toneMappingInfo)
    : Subrender(pipelineStage)
    , toneMappingInfo(toneMappingInfo)
    , pipeline(pipelineStage, {"Shader/ToneMapping/ToneMapping.vert", "Shader/ToneMapping/ToneMapping.frag"}, {}, {}, PipelineGraphics::Mode::Polygon,
               PipelineGraphics::Depth::None)
    , descriptorSet(pipeline)
{
    uniformToneMapping = UniformHandler(pipeline.GetShader()->GetUniformBlock("uniformToneMapping").value());
}

void ToneMappingSubrender::RegisterImGui()
{
    if (auto* imgui = Imgui::Get()) {
        Imgui::Get()->RegisterCustomWindow(typeid(*this).name(), [this]() {
            ImGui::SliderFloat("Exposure", &toneMappingInfo.exposure, 0.0f, 10.0f);
            ImGui::SameLine();
            ImGui::SliderFloat("Gamma", &toneMappingInfo.gamma, 0.0f, 10.0f);
        });
    }
}

void ToneMappingSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void ToneMappingSubrender::Render(const CommandBuffer& commandBuffer)
{
    // Updates storage buffers.
    uniformToneMapping.Push("exposure", toneMappingInfo.exposure);
    uniformToneMapping.Push("gamma", toneMappingInfo.gamma);

    descriptorSet.Push("uniformToneMapping", uniformToneMapping);
    descriptorSet.Push("ResolvedImage", Graphics::Get()->GetAttachment("resolve"));

    if (!descriptorSet.Update(pipeline)) return;
    pipeline.BindPipeline(commandBuffer);

    descriptorSet.BindDescriptor(commandBuffer, pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void ToneMappingSubrender::PostRender(const CommandBuffer& commandBuffer) {}

}   // namespace MapleLeaf