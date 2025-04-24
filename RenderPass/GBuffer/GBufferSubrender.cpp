#include "GBufferSubrender.hpp"
#include "GpuScene.hpp"
#include "Scenes.hpp"


namespace MapleLeaf {
GBufferSubrender::GBufferSubrender(const Pipeline::Stage& stage)
    : Subrender(stage)
    , compute("Shader/GPUDriven/Culling.comp")
    , pipeline(stage, {"Shader/GBuffer/GBuffer.vert", "Shader/GBuffer/GBuffer.frag"}, {Vertex3D::GetVertexInput()}, {}, PipelineGraphics::Mode::MRT,
               PipelineGraphics::Depth::ReadWrite, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
               VK_FRONT_FACE_COUNTER_CLOCKWISE, false)
    , descriptorSetCompute(compute)
    , descriptorSetGraphics(pipeline)
{
    pushHandler   = PushHandler(compute.GetShader()->GetUniformBlock("pushObject").value());
    uniformCamera = UniformHandler(compute.GetShader()->GetUniformBlock("camera").value(), true);
}

void GBufferSubrender::PreRender(const CommandBuffer& commandBuffer)
{
    const auto gpuScene = Scenes::Get()->GetScene()->GetDerivedScene<GPUScene>();
    if (!gpuScene || !gpuScene->GetIndirectBuffer()) return;

    auto camera = Scenes::Get()->GetScene()->GetCamera();
    camera->PushUniforms(uniformCamera);

    uint32_t instanceCount = gpuScene->GetInstanceCount();

    pushHandler.Push("instanceCount", instanceCount);

    descriptorSetCompute.Push("instanceDatas", gpuScene->GetInstanceDatasHandler());
    descriptorSetCompute.Push("drawCommandBuffer", gpuScene->GetIndirectBuffer());
    descriptorSetCompute.Push("camera", uniformCamera);
    descriptorSetCompute.Push("pushObject", pushHandler);

    if (!descriptorSetCompute.Update(compute)) return;
    compute.BindPipeline(commandBuffer);
    descriptorSetCompute.BindDescriptor(commandBuffer, compute);
    pushHandler.BindPush(commandBuffer, compute);
    compute.CmdRender(commandBuffer, glm::uvec2(instanceCount, 1));

    gpuScene->GetIndirectBuffer()->IndirectBufferPipelineBarrier(commandBuffer);
}

void GBufferSubrender::Render(const CommandBuffer& commandBuffer)
{
    const auto gpuScene = Scenes::Get()->GetScene()->GetDerivedScene<GPUScene>();
    if (!gpuScene) return;

    auto camera = Scenes::Get()->GetScene()->GetCamera();
    camera->PushUniforms(uniformCamera);

    descriptorSetGraphics.Push("camera", uniformCamera);
    gpuScene->PushDescriptors(descriptorSetGraphics);

    if (!descriptorSetGraphics.Update(pipeline)) return;
    pipeline.BindPipeline(commandBuffer);

    descriptorSetGraphics.BindDescriptor(commandBuffer, pipeline);

    gpuScene->CmdRender(commandBuffer);
}

void GBufferSubrender::PostRender(const CommandBuffer& commandBuffer) {}

void GBufferSubrender::RegisterImGui() {}

}   // namespace MapleLeaf
