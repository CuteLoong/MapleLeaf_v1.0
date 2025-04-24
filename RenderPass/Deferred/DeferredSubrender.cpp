#include "DeferredSubrender.hpp"
#include "LightSystem.hpp"
#include "Scenes.hpp"
#include "ShadowSystem.hpp"
#include "SkyboxSystem.hpp"

namespace MapleLeaf {

DeferredSubrender::DeferredSubrender(const Pipeline::Stage& pipelineStage)
    : Subrender(pipelineStage)
    , pipeline(pipelineStage, {"Shader/Deferred/Deferred.vert", "Shader/Deferred/Deferred.frag"}, {}, {}, PipelineGraphics::Mode::Polygon,
               PipelineGraphics::Depth::None)
    , descriptorSet(pipeline)
{
    // uniformScene  = UniformHandler(pipeline.GetShader()->GetUniformBlock("uniformScene").value());
    // uniformCamera = UniformHandler(pipeline.GetShader()->GetUniformBlock("camera").value());
}

void DeferredSubrender::RegisterImGui() {}

void DeferredSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void DeferredSubrender::Render(const CommandBuffer& commandBuffer)
{
    auto camera = Scenes::Get()->GetScene()->GetCamera();

    const auto& skybox      = Scenes::Get()->GetScene()->GetSystem<SkyboxSystem>();
    const auto& lightSystem = Scenes::Get()->GetScene()->GetSystem<LightSystem>();

    camera->PushUniforms(uniformCamera);

    if (auto shadows = Scenes::Get()->GetScene()->GetSystem<ShadowSystem>())
        uniformScene.Push("shadowMatrix", shadows->GetShadowCascade().GetLightProjectionViewMatrix());
    uniformScene.Push("pointLightsCount", lightSystem->GetPointLightsCount() - 1);
    uniformScene.Push("directionalLightsCount", lightSystem->GetDirectionalLightsCount() - 1);
    uniformScene.Push("areaLightsCount", lightSystem->GetAreaLightsCount() - 1);
    uniformScene.Push("skyboxLoaded", int(skybox->IsLoaded()));

    descriptorSet.Push("uniformScene", uniformScene);
    descriptorSet.Push("camera", uniformCamera);
    descriptorSet.Push("bufferPointLights", lightSystem->GetStoragePointLights());
    descriptorSet.Push("bufferDirectionalLights", lightSystem->GetStorageDirectionalLights());
    descriptorSet.Push("bufferAreaLights", lightSystem->GetStorageAreaLights());

    descriptorSet.Push("inPosition", Graphics::Get()->GetAttachment("position"));
    descriptorSet.Push("inDiffuse", Graphics::Get()->GetAttachment("diffuse"));
    descriptorSet.Push("inNormal", Graphics::Get()->GetAttachment("normal"));
    descriptorSet.Push("inMaterial", Graphics::Get()->GetAttachment("material"));

    descriptorSet.Push("inShadowMap", Graphics::Get()->GetAttachment("shadows"));

    if (skybox->IsLoaded()) {
        descriptorSet.Push("samplerBRDF", skybox->GetBRDF());
        descriptorSet.Push("samplerIrradiance", skybox->GetIrradiance());
        descriptorSet.Push("samplerPrefiltered", skybox->GetPrefiltered());
    }

    descriptorSet.Push("LTC1", lightSystem->GetLTCTexture1());
    descriptorSet.Push("LTC2", lightSystem->GetLTCTexture2());

    if (!descriptorSet.Update(pipeline)) return;

    // Draws the object.
    pipeline.BindPipeline(commandBuffer);

    descriptorSet.BindDescriptor(commandBuffer, pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void DeferredSubrender::PostRender(const CommandBuffer& commandBuffer) {}
}   // namespace MapleLeaf