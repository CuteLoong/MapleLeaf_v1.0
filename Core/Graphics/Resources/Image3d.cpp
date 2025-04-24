#include "Image3d.hpp"
#include "Buffer.hpp"
#include "ResourceFormat.h"


namespace MapleLeaf {
Image3d::Image3d(const glm::uvec3& extent, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkFilter filter,
                 VkSamplerAddressMode addressMode, VkSampleCountFlagBits samples, bool anisotropic, bool mipmap)
    : Image(filter, addressMode, samples, layout,
            usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, format, 1, 1,
            {extent.x, extent.y, extent.z})
    , anisotropic(anisotropic)
    , mipmap(mipmap)
    , components(4)
{
    Image3d::Load();
}

void Image3d::ClearImage3d(const glm::vec4& color) const
{
    CommandBuffer commandBuffer;

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = mipLevels;
    subresourceRange.baseArrayLayer          = 0;
    subresourceRange.layerCount              = arrayLayers;

    VkClearColorValue clearColor = {};
    std::memcpy(clearColor.float32, &color, sizeof(color));

    vkCmdClearColorImage(commandBuffer.GetCommandBuffer(), image, layout, &clearColor, 1, &subresourceRange);

    commandBuffer.SubmitIdle();
}

void Image3d::Load()
{
    if (extent.width == 0 || extent.height == 0 || extent.depth == 0) return;

    mipLevels = mipmap ? GetMipLevels(extent) : 1;

    CreateImage(image,
                memory,
                extent,
                format,
                samples,
                VK_IMAGE_TILING_OPTIMAL,
                usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                mipLevels,
                arrayLayers,
                VK_IMAGE_TYPE_3D);
    CreateImageSampler(sampler, filter, addressMode, anisotropic, mipLevels);
    CreateImageView(image, view, VK_IMAGE_VIEW_TYPE_3D, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

    if (mipLevels >= 1) {
        mipViews.resize(mipLevels, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < mipLevels; i++)
            CreateImageView(image, mipViews[i], VK_IMAGE_VIEW_TYPE_3D, format, VK_IMAGE_ASPECT_COLOR_BIT, 1, i, arrayLayers, 0);
    }

    TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
}

void Image3d::Image3dPipelineBarrierComputeToCompute(const CommandBuffer& commandBuffer, int mipLevel) const
{
    InsertImageMemoryBarrier(commandBuffer,
                             image,
                             VK_ACCESS_SHADER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             layout,
                             layout,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             mipLevel,
                             1,
                             0);
}

void Image3d::Image3dPipelineBarrierComputeToGraphic(const CommandBuffer& commandBuffer, int mipLevel) const
{
    InsertImageMemoryBarrier(commandBuffer,
                             image,
                             VK_ACCESS_SHADER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             layout,
                             layout,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             mipLevel,
                             1,
                             0);
}

void Image3d::Image3dPipelineBarrierGraphicToCompute(const CommandBuffer& commandBuffer, int mipLevel) const
{
    InsertImageMemoryBarrier(commandBuffer,
                             image,
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             layout,
                             layout,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             mipLevel,
                             1,
                             0);
}

}   // namespace MapleLeaf