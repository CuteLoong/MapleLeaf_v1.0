#include "RayTracingSubrender.hpp"

#include "ASScene.hpp"
#include "GPUScene.hpp"
#include "Imgui.hpp"
#include "LightSystem.hpp"
#include "Scenes.hpp"
#include "ShadowSystem.hpp"
#include "SkyboxSystem.hpp"

namespace MapleLeaf {
RayTracingSubrender::RayTracingSubrender(const Pipeline::Stage& pipelineStage)
    : Subrender(pipelineStage)
    , pipelineRayTracing({"RayTracing/RayTrace.rgen", "RayTracing/RayTrace.rmiss", "RayTracing/RayTrace.rchit"})
{
    // uniformFrameData = UniformHandler(pipelineRayTracing.GetShader()->GetUniformBlock("uniformFrameData").value(), true);
    // uniformGeometry  = UniformHandler(pipelineRayTracing.GetShader()->GetUniformBlock("uniformGeometry").value(), true);
    // uniformScene     = UniformHandler(pipelineRayTracing.GetShader()->GetUniformBlock("uniformScene").value(), true);
}

void RayTracingSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void RayTracingSubrender::Render(const CommandBuffer& commandBuffer) {}

void RayTracingSubrender::PostRender(const CommandBuffer& commandBuffer)
{
    auto gpuScene = Scenes::Get()->GetScene()->GetDerivedScene<GPUScene>();
    if (!gpuScene) return;

    const auto& skybox      = Scenes::Get()->GetScene()->GetSystem<SkyboxSystem>();
    const auto& lightSystem = Scenes::Get()->GetScene()->GetSystem<LightSystem>();
    const auto& AS          = Scenes::Get()->GetScene()->GetDerivedScene<ASScene>()->GetTopLevelAccelerationStruct();

    auto camera = Scenes::Get()->GetScene()->GetCamera();
    camera->PushUniforms(uniformCamera);

    if (auto shadows = Scenes::Get()->GetScene()->GetSystem<ShadowSystem>())
        uniformScene.Push("shadowMatrix", shadows->GetShadowCascade().GetLightProjectionViewMatrix());
    uniformScene.Push("pointLightsCount", lightSystem->GetPointLightsCount() - 1);
    uniformScene.Push("directionalLightsCount", lightSystem->GetDirectionalLightsCount() - 1);
    uniformScene.Push("areaLightsCount", lightSystem->GetAreaLightsCount() - 1);
    uniformScene.Push("skyboxLoaded", int(skybox->IsLoaded()));
    uniformScene.Push("wallroughness", wallroughness);

    uniformGeometry.Push("vertexAddress", gpuScene->GetVertexBuffer()->GetDeviceAddress());
    uniformGeometry.Push("indexAddress", gpuScene->GetIndexBuffer()->GetDeviceAddress());

    uniformFrameData.Push("frameID", camera->frameID);
    uniformFrameData.Push("spp", spp);
    uniformFrameData.Push("maxDepth", maxDepth);

    descriptorSet.Push("topLevelAS", AS);
    descriptorSet.Push("uniformFrameData", uniformFrameData);
    descriptorSet.Push("uniformGeometry", uniformGeometry);
    descriptorSet.Push("uniformScene", uniformScene);
    descriptorSet.Push("camera", uniformCamera);
    descriptorSet.Push("bufferPointLights", lightSystem->GetStoragePointLights());
    descriptorSet.Push("bufferDirectionalLights", lightSystem->GetStorageDirectionalLights());
    descriptorSet.Push("bufferAreaLights", lightSystem->GetStorageAreaLights());

    if (skybox->IsLoaded()) {
        descriptorSet.Push("SkyboxCubeMap", skybox->GetSkybox());
        descriptorSet.Push("samplerBRDF", skybox->GetBRDF());
        descriptorSet.Push("samplerIrradiance", skybox->GetIrradiance());
        descriptorSet.Push("samplerPrefiltered", skybox->GetPrefiltered());
    }
    else {
        descriptorSet.Push("SkyboxCubeMap", skybox->GetImageCubePlaceholder());
        descriptorSet.Push("samplerBRDF", skybox->GetImage2dPlaceholder());
        descriptorSet.Push("samplerIrradiance", skybox->GetImageCubePlaceholder());
        descriptorSet.Push("samplerPrefiltered", skybox->GetImageCubePlaceholder());
    }

    descriptorSet.Push("LTC1", lightSystem->GetLTCTexture1());
    descriptorSet.Push("LTC2", lightSystem->GetLTCTexture2());

    gpuScene->PushDescriptors(descriptorSet);

    descriptorSet.Push("image", Graphics::Get()->GetAttachment("RayTracingTarget"));

    if (!descriptorSet.Update(pipelineRayTracing)) return;

    pipelineRayTracing.BindPipeline(commandBuffer);
    descriptorSet.BindDescriptor(commandBuffer, pipelineRayTracing);
    pipelineRayTracing.CmdRender(commandBuffer, Devices::Get()->GetWindow()->GetSize());
}

void RayTracingSubrender::RegisterImGui()
{
    if (auto* imgui = Imgui::Get()) {
        imgui->RegisterCustomWindow(typeid(*this).name(), [this]() {
            ImGui::SliderFloat("Wall Roughness", &wallroughness, 0.0f, 1.0f);

            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputInt("Samples Per Pixel", &spp);
            ImGui::InputInt("Max Depth", &maxDepth);
        });
    }
}
}   // namespace MapleLeaf