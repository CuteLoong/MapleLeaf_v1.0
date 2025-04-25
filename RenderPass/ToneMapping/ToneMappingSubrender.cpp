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
            ImGui::SetNextItemWidth(100.0f);
            ImGui::SliderFloat("Exposure", &toneMappingInfo.exposure, 0.0f, 10.0f);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(100.0f);
            ImGui::SliderFloat("Gamma", &toneMappingInfo.gamma, 0.0f, 10.0f);

            ImGui::SetNextItemWidth(200.0f);
            ImGui::SliderFloat("whiteMaxLuminance", &toneMappingInfo.whiteMaxLuminance, 0.0f, 10.0f);

            ImGui::SetNextItemWidth(200.0f);
            ImGui::SliderFloat("whiteScale", &toneMappingInfo.whiteScale, 0.0f, 100.0f);

            // Add tone mapping type dropdown
            ImGui::SetNextItemWidth(150.0f);
            static const char* typeNames[] = {"Linear", "Reinhard", "Reinhard Modified", "Heji Hable Alu", "HableUc2", "Aces"};
            int                currentItem = static_cast<int>(toneMappingInfo.type);
            if (ImGui::Combo("Tone Mapping Type", &currentItem, typeNames, IM_ARRAYSIZE(typeNames))) {
                toneMappingInfo.type = static_cast<Type>(currentItem);
            }
        });
    }
}

void ToneMappingSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void ToneMappingSubrender::Render(const CommandBuffer& commandBuffer)
{
    // Updates storage buffers.
    uniformToneMapping.Push("exposure", toneMappingInfo.exposure);
    uniformToneMapping.Push("gamma", toneMappingInfo.gamma);
    uniformToneMapping.Push("whiteMaxLuminance", toneMappingInfo.whiteMaxLuminance);
    uniformToneMapping.Push("whiteScale", toneMappingInfo.whiteScale);
    uniformToneMapping.Push("type", static_cast<int>(toneMappingInfo.type));

    descriptorSet.Push("uniformToneMapping", uniformToneMapping);
    descriptorSet.Push("ResolvedImage", Graphics::Get()->GetAttachment("resolve"));

    if (!descriptorSet.Update(pipeline)) return;
    pipeline.BindPipeline(commandBuffer);

    descriptorSet.BindDescriptor(commandBuffer, pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void ToneMappingSubrender::PostRender(const CommandBuffer& commandBuffer) {}

}   // namespace MapleLeaf