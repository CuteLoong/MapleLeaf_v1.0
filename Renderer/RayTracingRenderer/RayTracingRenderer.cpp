#include "RayTracingRenderer.hpp"
#include "ImguiSubrender.hpp"
#include "RayTracingSubrender.hpp"
#include "RenderStage.hpp"
#include "ToneMappingSubrender.hpp"
#include "config.h"

namespace MapleLeafApp {
RayTracingRenderer::RayTracingRenderer()
{
    std::vector<Attachment>  rayTracingAttachments = {{0, "RayTracingTarget", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT},
                                                      {1, "PlaceHolder", Attachment::Type::FrameBuffer, false, VK_FORMAT_R8G8B8A8_UNORM}};
    std::vector<SubpassType> rayTracingSubpasses   = {{0, {}, {0}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, rayTracingAttachments, rayTracingSubpasses));

    std::vector<Attachment>  ToneMappingAttachments = {{0, "swapchain", Attachment::Type::Swapchain, false}};
    std::vector<SubpassType> ToneMappingSubpasses   = {{0, {}, {0}}, {1, {}, {0}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, ToneMappingAttachments, ToneMappingSubpasses));
}

void RayTracingRenderer::Start()
{
    AddSubrender<RayTracingSubrender>({0, 0});
    AddSubrender<ToneMappingSubrender>({1, 0}, "RayTracingTarget");
    AddSubrender<ImguiSubrender>({1, 1});
}

void RayTracingRenderer::Update() {}

}   // namespace MapleLeafApp
