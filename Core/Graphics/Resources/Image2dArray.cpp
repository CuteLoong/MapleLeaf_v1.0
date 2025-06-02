#include "Image2dArray.hpp"
#include "Buffer.hpp"

namespace MapleLeaf {
Image2dArray::Image2dArray(const glm::uvec2& extent, uint32_t arrayLayers, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage,
                           VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, bool mipmap)
    : Image(filter, addressMode, VK_SAMPLE_COUNT_1_BIT, layout,
            usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, format, 1, arrayLayers,
            {extent.x, extent.y, 1})
    , anisotropic(anisotropic)
    , mipmap(mipmap)
{
    Image2dArray::Load();
}

Image2dArray::Image2dArray(std::unique_ptr<Bitmap>&& bitmap, uint32_t arrayLayers, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage,
                           VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic, bool mipmap)
    : Image(filter, addressMode, VK_SAMPLE_COUNT_1_BIT, layout,
            usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, format, 1, arrayLayers,
            {bitmap->GetSize().x, bitmap->GetSize().y, 1})
    , anisotropic(anisotropic)
    , mipmap(mipmap)
{
    if (extent.width == 0 || extent.height == 0) {
        return;
    }

    // mipLevels = mipmap ? GetMipLevels(extent) : 1;

    CreateImage(image,
                memory,
                extent,
                format,
                samples,
                VK_IMAGE_TILING_OPTIMAL,
                this->usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                mipLevels,
                arrayLayers,
                VK_IMAGE_TYPE_2D);
    CreateImageSampler(sampler, filter, addressMode, anisotropic, mipLevels);
    CreateImageView(image, view, VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

    if (mipLevels >= 1) {
        mipViews.resize(mipLevels, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < mipLevels; i++)
            CreateImageView(image, mipViews[i], VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, VK_IMAGE_ASPECT_COLOR_BIT, 1, i, arrayLayers, 0);
    }

    TransitionImageLayout(
        image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

    Buffer bufferStaging(bitmap->GetLength() * arrayLayers,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    uint8_t* data;
    bufferStaging.MapMemory(reinterpret_cast<void**>(&data));
    std::memcpy(data, bitmap->GetData().get(), bufferStaging.GetSize());
    bufferStaging.UnmapMemory();

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    bufferCopyRegions.reserve(arrayLayers);
    for (uint32_t layer = 0; layer < arrayLayers; layer++) {
        VkBufferImageCopy region               = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = extent;
        region.bufferOffset                    = 3 * extent.width * extent.height * layer;
        bufferCopyRegions.emplace_back(region);
    }
    CommandBuffer commandBuffer;
    vkCmdCopyBufferToImage(
        commandBuffer, bufferStaging.GetBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bufferCopyRegions.size(), bufferCopyRegions.data());
    commandBuffer.SubmitIdle();

    TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
}

void Image2dArray::Load(std::unique_ptr<Bitmap> loadBitmap)
{
    if (extent.width == 0 || extent.height == 0) return;

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
                VK_IMAGE_TYPE_2D);
    CreateImageSampler(sampler, filter, addressMode, anisotropic, mipLevels);
    CreateImageView(image, view, VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

    if (mipLevels >= 1) {
        mipViews.resize(mipLevels, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < mipLevels; i++)
            CreateImageView(image, mipViews[i], VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, VK_IMAGE_ASPECT_COLOR_BIT, 1, i, arrayLayers, 0);
    }

    if (loadBitmap || mipmap) {
        TransitionImageLayout(
            image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
    }

    if (loadBitmap) {
        Buffer bufferStaging(
            loadBitmap->GetLength(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        uint8_t* data;
        bufferStaging.MapMemory(reinterpret_cast<void**>(&data));
        std::memcpy(data, loadBitmap->GetData().get(), bufferStaging.GetSize());
        bufferStaging.UnmapMemory();

        CopyBufferToImage(bufferStaging.GetBuffer(), image, extent, arrayLayers, 0);
    }

    if (mipmap) {
        CreateMipmaps(image, extent, format, layout, mipLevels, 0, arrayLayers);
    }
    else if (loadBitmap) {
        TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
    }
    else {
        TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
    }
}

void Image2dArray::SetPixels(const float* pixels, uint32_t arrayLayer)
{
    Buffer bufferStaging(extent.width * extent.height * 3,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    bufferStaging.MapMemory(&data);
    std::memcpy(data, pixels, bufferStaging.GetSize());
    bufferStaging.UnmapMemory();

    CopyBufferToImage(bufferStaging.GetBuffer(), image, extent, 1, arrayLayer);
}

void Image2dArray::ClearImage2dArray(const CommandBuffer& commandBuffer, const glm::vec4& color, std::optional<uint32_t> layerBase,
                                     std::optional<uint32_t> layerCount) const
{
    InsertImageMemoryBarrier(commandBuffer,
                             image,
                             0,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             layout,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             mipLevels,
                             0,
                             arrayLayers,
                             0);

    VkClearColorValue       clearColor = {color.r, color.g, color.b, color.a};
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = mipLevels;
    subresourceRange.baseArrayLayer = layerBase.value_or(0);
    subresourceRange.layerCount     = layerCount.value_or(arrayLayers);

    vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &subresourceRange);

    InsertImageMemoryBarrier(commandBuffer,
                             image,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             layout,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             mipLevels,
                             0,
                             arrayLayers,
                             0);
}

void Image2dArray::ClearImage2dArray(const glm::vec4& color, std::optional<uint32_t> layerBase, std::optional<uint32_t> layerCount) const
{
    CommandBuffer commandBuffer;
    ClearImage2dArray(commandBuffer, color, layerBase, layerCount);

    commandBuffer.SubmitIdle();
}

void Image2dArray::Image2dArrayPipelineBarrierRayTracingToCompute(const CommandBuffer& commandBuffer, uint32_t mipLevel) const
{
    InsertImageMemoryBarrier(commandBuffer,
                             image,
                             VK_ACCESS_SHADER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             layout,
                             layout,
                             VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             mipLevel,
                             arrayLayers,
                             0);
}

void Image2dArray::Image2dArrayPipelineBarrierComputeToCompute(const CommandBuffer& commandBuffer, uint32_t mipLevel) const
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
                             arrayLayers,
                             0);
}

void Image2dArray::Image2dArrayPipelineBarrierComputeToGraphic(const CommandBuffer& commandBuffer, uint32_t mipLevel) const
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
                             arrayLayers,
                             0);
}

void Image2dArray::Image2dArrayPipelineBarrierGraphicToCompute(const CommandBuffer& commandBuffer, uint32_t mipLevel) const
{
    InsertImageMemoryBarrier(commandBuffer,
                             image,
                             VK_ACCESS_SHADER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             layout,
                             layout,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             mipLevel,
                             arrayLayers,
                             0);
}
}   // namespace MapleLeaf
