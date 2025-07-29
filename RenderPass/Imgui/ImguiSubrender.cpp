#include "ImguiSubrender.hpp"
#include "Imgui.hpp"

namespace MapleLeaf {
ImguiSubrender::ImguiSubrender(const Pipeline::Stage& pipelineStage)
    : Subrender(pipelineStage)
{}

void ImguiSubrender::RegisterImGui()
{
    if (auto* imgui = Imgui::Get()) {}
}

void ImguiSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void ImguiSubrender::Render(const CommandBuffer& commandBuffer)
{
    Imgui::Get()->Render(commandBuffer);
}

void ImguiSubrender::PostRender(const CommandBuffer& commandBuffer) {}
}   // namespace MapleLeaf