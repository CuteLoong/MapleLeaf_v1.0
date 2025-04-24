#include "SkyboxSubrender.hpp"

#include "Scenes.hpp"
#include "SkyboxSystem.hpp"

namespace MapleLeaf {
static const Color SKYBOX_COLOUR_DAY(0x003C8A);

SkyboxSubrender::SkyboxSubrender(const Pipeline::Stage& pipelineStage)
    : Subrender(pipelineStage)
    , pipelineGraphics(pipelineStage, {"Shader/Skybox/Skybox.vert", "Shader/Skybox/Skybox.frag"}, {}, {}, PipelineGraphics::Mode::MRT,
                       PipelineGraphics::Depth::None, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                       VK_FRONT_FACE_COUNTER_CLOCKWISE, false)
    , descriptorSet(pipelineGraphics)
{
    uniformSkybox = UniformHandler(pipelineGraphics.GetShader()->GetUniformBlock("uniformSkybox").value(), false);
    uniformCamera = UniformHandler(pipelineGraphics.GetShader()->GetUniformBlock("camera").value(), false);
}

void SkyboxSubrender::RegisterImGui() {}

void SkyboxSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void SkyboxSubrender::Render(const CommandBuffer& commandBuffer)
{
    const auto& skybox = Scenes::Get()->GetScene()->GetSystem<SkyboxSystem>();
    if (!skybox || !skybox->IsLoaded()) return;

    auto camera = Scenes::Get()->GetScene()->GetCamera();
    camera->PushUniforms(uniformCamera);

    uniformSkybox.Push("transform", skybox->GetTransform()->GetWorldMatrix());
    uniformSkybox.Push("baseColour", SKYBOX_COLOUR_DAY);
    uniformSkybox.Push("blendFactor", 1.0f);

    descriptorSet.Push("camera", uniformCamera);
    descriptorSet.Push("uniformSkybox", uniformSkybox);
    descriptorSet.Push("SkyboxCubeMap", skybox->GetSkybox());

    if (!descriptorSet.Update(pipelineGraphics)) return;

    pipelineGraphics.BindPipeline(commandBuffer);

    descriptorSet.BindDescriptor(commandBuffer, pipelineGraphics);

    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void SkyboxSubrender::PostRender(const CommandBuffer& commandBuffer) {}

}   // namespace MapleLeaf