#include "SkyboxMappingSubrender.hpp"
#include "Scenes.hpp"
#include "SkyboxSystem.hpp"

namespace MapleLeaf {
SkyboxMappingSubrender::SkyboxMappingSubrender(const Pipeline::Stage& pipelineStage)
    : Subrender(pipelineStage)
    , pipelineGraphics(pipelineStage, {"Shader/Skybox/SkyboxMapping.vert", "Shader/Skybox/SkyboxMapping.frag"}, {}, {},
                       PipelineGraphics::Mode::Polygon, PipelineGraphics::Depth::None, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL,
                       VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, false)
{
    maxMipLevel = Image::GetMipLevels({1024, 1024});

    if (const auto* shader = pipelineGraphics.GetShader()) {
        uniformSkybox.reserve(maxMipLevel * 6);
        descriptorSet.reserve(maxMipLevel * 6);

        for (size_t i = 0; i < maxMipLevel * 6; ++i) {
            uniformSkybox.emplace_back(shader->GetUniformBlock("uniformSkybox").value(), true);
            descriptorSet.emplace_back(pipelineGraphics);
        }
    }
}

void SkyboxMappingSubrender::PreRender(const CommandBuffer& commandBuffer) {}

void SkyboxMappingSubrender::Render(const CommandBuffer& commandBuffer)
{
    const auto& skyboxSystem = Scenes::Get()->GetScene()->GetSystem<SkyboxSystem>();
    if (!skyboxSystem) return;
    if (!skyboxSystem->WaitMapping()) return;

    std::vector<glm::mat4> matrices = {
        // POSITIVE_X
        glm::rotate(
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_X
        glm::rotate(
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
    };
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    const auto& MappedSkybox = dynamic_cast<const Image2d*>(Graphics::Get()->GetAttachment("MappedSkybox"));
    const auto& SkyboxCube   = skyboxSystem->GetSkybox();

    for (int i = 0; i < maxMipLevel; i++) {
        for (int j = 0; j < 6; j++) {
            glm::uvec2 extent = glm::uvec2(1024 >> i, 1024 >> i);

            VkViewport viewport = {};
            viewport.x          = 0.0f;
            viewport.y          = static_cast<float>(extent.y);
            viewport.width      = static_cast<float>(extent.x);
            viewport.height     = -static_cast<float>(extent.y);
            viewport.minDepth   = 0.0f;
            viewport.maxDepth   = 1.0f;

            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            uniformSkybox[i * 6 + j].Push("view", matrices[j]);
            uniformSkybox[i * 6 + j].Push("projection", projection);
            uniformSkybox[i * 6 + j].Push("index", j);

            uniformParams.Push("gamma", skyboxSystem->GetGamma());
            uniformParams.Push("exposure", skyboxSystem->GetExposure());

            descriptorSet[i * 6 + j].Push("uniformParams", uniformParams);
            descriptorSet[i * 6 + j].Push("uniformSkybox", uniformSkybox[i * 6 + j]);
            descriptorSet[i * 6 + j].Push("EquirectangularMap", skyboxSystem->GetSkyboxEqMap());
            descriptorSet[i * 6 + j].Push("CubeMap", SkyboxCube, i, std::nullopt, std::nullopt);

            if (!descriptorSet[i * 6 + j].Update(pipelineGraphics)) continue;

            pipelineGraphics.BindPipeline(commandBuffer);
            descriptorSet[i * 6 + j].BindDescriptor(commandBuffer, pipelineGraphics);
            vkCmdDraw(commandBuffer, 36, 1, 0, 0);
        }
    }
    skyboxSystem->SetMapped(true);
}

void SkyboxMappingSubrender::PostRender(const CommandBuffer& commandBuffer) {}

void SkyboxMappingSubrender::RegisterImGui() {}

}   // namespace MapleLeaf