#include "SkyboxSystem.hpp"

#include "DescriptorHandler.hpp"
#include "Graphics.hpp"
#include "Imgui.hpp"
#include "PipelineCompute.hpp"
#include "Resources.hpp"

namespace MapleLeaf {
SkyboxSystem::SkyboxSystem()
    : gamma(2.2f)
    , exposure(1.0f)
    , rotation(0.0f, 0.0f, 0.0f)
{
    brdf   = Resources::Get()->GetThreadPool().Enqueue(ComputeBRDF, 512);
    loaded = false;
}

bool SkyboxSystem::WaitMapping() const
{
    if (skybox)
        return !skybox->GetMapped();
    else
        return false;
}

void SkyboxSystem::RegisterCustomWindow()
{
    if (auto* imgui = Imgui::Get()) {
        imgui->RegisterCustomWindow(typeid(*this).name(), [this]() {
            if (skybox.get() == nullptr) {
                ImGui::Text("Skybox: ");

                if (ImGui::Button("Load Skybox")) {
                    std::string skyboxPath;
                    if (Imgui::Get()->OpenFileDialog(skyboxPath, "EXR Files\0*.exr\0", "Load Skybox")) {
                        skybox = std::make_unique<Skybox>(skyboxPath);
                        loaded = false;
                    }
                }
            }
            else {
                ImGui::Text("Skybox: %s", skybox->GetFilename().c_str());

                if (ImGui::Button("ReLoad Skybox")) {
                    std::string skyboxPath;
                    if (Imgui::Get()->OpenFileDialog(skyboxPath, "EXR Files\0*.exr\0", "Load Skybox")) {
                        skybox = std::make_unique<Skybox>(skyboxPath);
                        loaded = false;
                    }
                }



                // ImGui::SetNextItemWidth(100.0f);
                // if (ImGui::SliderFloat("Gamma", &gamma, 0.0f, 5.0f, "%.2f")) {
                //     loaded = false;
                //     skybox->SetMapped(false);
                // }

                ImGui::SetNextItemWidth(300.0f);
                if (ImGui::SliderFloat3("Rotation", &rotation.x, 0.0f, 360.0f, "%.2f")) {
                    skybox->SetRotation(rotation);
                }
            }
        });
    }
}

void SkyboxSystem::Update()
{
    RegisterCustomWindow();
    ComputeSkybox();
}

void SkyboxSystem::ComputeSkybox()
{
    if (skybox != nullptr && skybox->GetMapped() && !loaded) {
        irradiance  = Resources::Get()->GetThreadPool().Enqueue(ComputeIrradiance, skybox->GetImageCube(), 64);
        prefiltered = Resources::Get()->GetThreadPool().Enqueue(ComputePrefiltered, skybox->GetImageCube(), 512);
        loaded      = true;
        // irradiance  = ComputeIrradiance(skybox->GetImageCube(), 64);
        // prefiltered = ComputePrefiltered(skybox->GetImageCube(), 512);
    }
}

std::unique_ptr<Image2d> SkyboxSystem::ComputeBRDF(uint32_t size)
{
    auto brdfImage = std::make_unique<Image2d>(glm::uvec2(size), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_GENERAL);

    CommandBuffer   commandBuffer(true, VK_QUEUE_COMPUTE_BIT);
    PipelineCompute compute("Shader/Skybox/PreIntegrationDFG.comp");

    // Bind the pipeline.
    compute.BindPipeline(commandBuffer);

    // Updates descriptors.
    DescriptorsHandler descriptorSet(compute);
    descriptorSet.Push("preIntegratedDFG", brdfImage.get());
    descriptorSet.Update(compute);

    // Runs the compute pipeline.
    descriptorSet.BindDescriptor(commandBuffer, compute);
    compute.CmdRender(commandBuffer, brdfImage->GetSize());
    commandBuffer.SubmitIdle();

    return brdfImage;
}

std::unique_ptr<ImageCube> SkyboxSystem::ComputeIrradiance(const ImageCube* source, uint32_t size)
{
    if (!source) {
        return nullptr;
    }

    auto irradianceCubemap = std::make_unique<ImageCube>(glm::ivec2(size), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_GENERAL);

    // Creates the pipeline.
    CommandBuffer   commandBuffer(true, VK_QUEUE_COMPUTE_BIT);
    PipelineCompute compute("Shader/Skybox/Irradiance.comp");

    // Bind the pipeline.
    compute.BindPipeline(commandBuffer);

    // Updates descriptors.
    DescriptorsHandler descriptorSet(compute);
    descriptorSet.Push("outColour", irradianceCubemap.get());
    descriptorSet.Push("samplerColour", source);
    descriptorSet.Update(compute);

    // Runs the compute pipeline.
    descriptorSet.BindDescriptor(commandBuffer, compute);
    compute.CmdRender(commandBuffer, irradianceCubemap->GetSize());
    commandBuffer.SubmitIdle();

    return irradianceCubemap;
}

std::unique_ptr<ImageCube> SkyboxSystem::ComputePrefiltered(const ImageCube* source, uint32_t size)
{
    if (!source) {
        return nullptr;
    }

    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    auto prefilteredCubemap = std::make_unique<ImageCube>(glm::ivec2(size),
                                                          VK_FORMAT_R16G16B16A16_SFLOAT,
                                                          VK_IMAGE_LAYOUT_GENERAL,
                                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                                          VK_FILTER_LINEAR,
                                                          VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                          VK_SAMPLE_COUNT_1_BIT,
                                                          true,
                                                          true);

    // Creates the pipeline.
    CommandBuffer   commandBuffer(true, VK_QUEUE_COMPUTE_BIT);
    PipelineCompute compute("Shader/Skybox/Prefiltered.comp");

    DescriptorsHandler descriptorSet(compute);
    PushHandler        pushHandler(*compute.GetShader()->GetUniformBlock("pushObject"));

    // TODO: Use image barriers between rendering (single command buffer), rework write descriptor passing. Image class also needs a restructure.
    for (uint32_t i = 0; i < prefilteredCubemap->GetMipLevels(); i++) {
        VkImageView levelView = VK_NULL_HANDLE;
        Image::CreateImageView(prefilteredCubemap->GetImage(),
                               levelView,
                               VK_IMAGE_VIEW_TYPE_CUBE,
                               prefilteredCubemap->GetFormat(),
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               1,
                               i,
                               6,
                               0);

        commandBuffer.Begin();
        compute.BindPipeline(commandBuffer);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.sampler               = prefilteredCubemap->GetSampler();
        imageInfo.imageView             = levelView;
        imageInfo.imageLayout           = prefilteredCubemap->GetLayout();

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet               = VK_NULL_HANDLE;   // Will be set in the descriptor handler.
        descriptorWrite.dstBinding           = *compute.GetShader()->GetDescriptorLocation("outColour").second;
        descriptorWrite.dstArrayElement      = 0;
        descriptorWrite.descriptorCount      = 1;
        descriptorWrite.descriptorType =
            *compute.GetShader()->GetDescriptorType(*compute.GetShader()->GetDescriptorLocation("outColour").first, descriptorWrite.dstBinding);
        // descriptorWrite.pImageInfo = &imageInfo;
        WriteDescriptorSet writeDescriptorSet(descriptorWrite, imageInfo);

        pushHandler.Push("roughness", static_cast<float>(i) / static_cast<float>(prefilteredCubemap->GetMipLevels() - 1));

        descriptorSet.Push("pushObject", pushHandler);
        descriptorSet.Push("outColour", prefilteredCubemap.get(), std::move(writeDescriptorSet));
        descriptorSet.Push("samplerColour", source);
        descriptorSet.Update(compute);

        descriptorSet.BindDescriptor(commandBuffer, compute);
        pushHandler.BindPush(commandBuffer, compute);
        compute.CmdRender(commandBuffer, prefilteredCubemap->GetSize() >> i);
        commandBuffer.SubmitIdle();

        vkDestroyImageView(*logicalDevice, levelView, nullptr);
    }

    return prefilteredCubemap;
}
}   // namespace MapleLeaf