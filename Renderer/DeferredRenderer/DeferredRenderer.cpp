#include "DeferredRenderer.hpp"

#include "DeferredSubrender.hpp"
#include "GBufferSubrender.hpp"
#include "ImguiSubrender.hpp"
#include "RenderStage.hpp"
#include "ResolvedSubrender.hpp"
#include "SkyboxSubrender.hpp"
#include "ToneMappingSubrender.hpp"

#include "config.h"


namespace MapleLeafApp {
DeferredRenderer::DeferredRenderer()
{
    // Render Pass for shadow map
    std::vector<Attachment>  ShadowAttachments = {{0, "ShadowMap", Attachment::Type::Depth, false}};
    std::vector<SubpassType> ShadowSubpasses   = {{0, {}, {0}}};
    AddRenderStage(
        std::make_unique<RenderStage>(RenderStage::Type::MONO, ShadowAttachments, ShadowSubpasses, Viewport({SHADOW_MAP_SIZE, SHADOW_MAP_SIZE})));

    // Render Pass for G-Buffer
    std::vector<Attachment>  GBufferAttachments = {{0, "depth", Attachment::Type::Depth, false},
                                                   {1, "position", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT},
                                                   {2, "diffuse", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT},
                                                   {3, "normal", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT},
                                                   {4, "material", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT},
                                                   {5, "motionVector", Attachment::Type::Image, false, VK_FORMAT_R32G32_SFLOAT},
                                                   {6, "instanceId", Attachment::Type::Image, false, VK_FORMAT_R32_SFLOAT, VK_FILTER_NEAREST}};
    std::vector<SubpassType> GBufferSubpasses   = {{0, {}, {1, 2, 3, 4}}, {1, {}, {0, 1, 2, 3, 4, 5, 6}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, GBufferAttachments, GBufferSubpasses));

    // Render Pass for Lighting
    std::vector<Attachment>  LightingAttachments = {{0, "lighting", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT}};
    std::vector<SubpassType> LightingSubpasses   = {{0, {}, {0}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, LightingAttachments, LightingSubpasses));

    // Render Pass for Resolve
    std::vector<Attachment>  ResolveAttachments = {{0, "resolve", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT}};
    std::vector<SubpassType> ResolveSubpasses   = {{0, {}, {0}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, ResolveAttachments, ResolveSubpasses));

    // Render Pass for Present
    std::vector<Attachment>  ToneMappingAttachments = {{0, "swapchain", Attachment::Type::Swapchain, false}};
    std::vector<SubpassType> ToneMappingSubpasses   = {{0, {}, {0}}, {1, {}, {0}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, ToneMappingAttachments, ToneMappingSubpasses));
}

void DeferredRenderer::Start()
{
    // AddSubrender<ShadowSubrender>({0, 0});
    AddSubrender<SkyboxSubrender>({1, 0});
    AddSubrender<GBufferSubrender>({1, 1});
    AddSubrender<DeferredSubrender>({2, 0});
    AddSubrender<ResolvedSubrender>({3, 0});
    AddSubrender<ToneMappingSubrender>({4, 0});
    AddSubrender<ImguiSubrender>({4, 1});
    // AddSubrender<DefaultSubrender>({4, 0});
    // AddSubrender<ImguiSubrender>({4, 1});
}

void DeferredRenderer::Update() {}

}   // namespace MapleLeafApp
