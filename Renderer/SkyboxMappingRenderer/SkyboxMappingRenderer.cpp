#include "SkyboxMappingRenderer.hpp"
#include "DefaultSubrender.hpp"
#include "ImguiSubrender.hpp"
#include "RenderStage.hpp"
#include "SkyboxMapping/SkyboxMappingSubrender.hpp"


namespace MapleLeafApp {
SkyboxMappingRenderer::SkyboxMappingRenderer()
{
    std::vector<Attachment>  skyboxMappingAttachments = {{0, "MappedSkybox", Attachment::Type::Image, false, VK_FORMAT_R32G32B32A32_SFLOAT}};
    std::vector<SubpassType> subpasses                = {{0, {}, {0}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, skyboxMappingAttachments, subpasses, Viewport({1024, 1024})));

    std::vector<Attachment>  attachments1{{0, "swapchain", Attachment::Type::Swapchain, false}};
    std::vector<SubpassType> subpasses1{{0, {}, {0}}, {1, {}, {0}}};

    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, attachments1, subpasses1));
}

void SkyboxMappingRenderer::Start()
{
    AddSubrender<SkyboxMappingSubrender>({0, 0});

    AddSubrender<DefaultSubrender>({1, 0});
    AddSubrender<ImguiSubrender>({1, 1});
}

void SkyboxMappingRenderer::Update() {}

}   // namespace MapleLeafApp