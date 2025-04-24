#include "DefaultRenderer.hpp"
#include "DefaultSubrender.hpp"
#include "ImguiSubrender.hpp"
#include "RenderStage.hpp"

namespace MapleLeafApp {

DefaultRenderer::DefaultRenderer()
{
    std::vector<Attachment>  attachments1{{0, "swapchain", Attachment::Type::Swapchain, false}};
    std::vector<SubpassType> subpasses1{{0, {}, {0}}, {1, {}, {0}}};

    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, attachments1, subpasses1));
}

void DefaultRenderer::Start()
{
    AddSubrender<DefaultSubrender>({0, 0});
    AddSubrender<ImguiSubrender>({0, 1});
}

void DefaultRenderer::Update() {}

}   // namespace MapleLeafApp