#include "ImageCube.hpp"
#include "Bitmap.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include <cassert>

namespace MapleLeaf {
std::shared_ptr<ImageCube> ImageCube::Create(const std::filesystem::path& filename, std::string fileSuffix, VkFilter filter,
                                             VkSamplerAddressMode addressMode, bool anisotropic, bool mipmap)
{
    auto result = std::make_shared<ImageCube>(filename, fileSuffix, filter, addressMode, anisotropic, mipmap, false);
    result->Load();
    return result;
}

ImageCube::ImageCube(const glm::uvec2& extent, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, VkFilter filter,
                     VkSamplerAddressMode addressMode, VkSampleCountFlagBits samples, bool anisotropic, bool mipmap)
    : Image(filter, addressMode, samples, layout,
            usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, format, 1, 6,
            {extent.x, extent.y, 1})
    , anisotropic(anisotropic)
    , mipmap(mipmap)
    , components(4)
{
    ImageCube::Load();
}

ImageCube::ImageCube(std::filesystem::path filename, std::string fileSuffix, VkFilter filter, VkSamplerAddressMode addressMode, bool anisotropic,
                     bool mipmap, bool load)
    : Image(filter, addressMode, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_FORMAT_R8G8B8A8_UNORM, 1, 6, {0, 0, 1})
    , filename(std::move(filename))
    , anisotropic(anisotropic)
    , fileSuffix(std::move(fileSuffix))
    , mipmap(mipmap)
{
    if (load) {
        ImageCube::Load();
    }
}

void ImageCube::Load(std::unique_ptr<Bitmap> loadBitmap)
{
    if (!filename.empty() && !loadBitmap) {
        uint8_t* offset = nullptr;
        for (const auto& side : fileSides) {
            Bitmap bitmapSide(filename / (side + fileSuffix));
            auto   lengthSide = bitmapSide.GetLength();

            if (!loadBitmap) {
                loadBitmap =
                    std::make_unique<Bitmap>(std::make_unique<uint8_t[]>(lengthSide * arrayLayers), bitmapSide.GetSize(), bitmapSide.GetFormat());
                offset = loadBitmap->GetData().get();
                format = GetVulkanFormat(loadBitmap->GetFormat());
            }

            std::memcpy(offset, bitmapSide.GetData().get(), lengthSide);
            offset += lengthSide;
        }
        extent     = {loadBitmap->GetSize().y, loadBitmap->GetSize().y, 1};
        components = loadBitmap->GetComponentCount();
    }

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
    CreateImageView(image, view, VK_IMAGE_VIEW_TYPE_CUBE, format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

    if (mipLevels >= 1) {
        mipViews.resize(mipLevels, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < mipLevels; i++)
            CreateImageView(image, mipViews[i], VK_IMAGE_VIEW_TYPE_CUBE, format, VK_IMAGE_ASPECT_COLOR_BIT, 1, i, arrayLayers, 0);
    }

    if (loadBitmap || mipmap) {
        TransitionImageLayout(
            image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
    }

    if (loadBitmap) {
        Buffer bufferStaging(loadBitmap->GetLength() * arrayLayers,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

void ImageCube::ImageCubePipelineBarrierComputeToCompute(const CommandBuffer& commandBuffer, std::optional<int> mipLevel) const
{
    if (mipLevel) {
        // Transition image from previous usage to transfer source layout
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
                                 mipLevel.value(),
                                 arrayLayers,
                                 0);
    }
    else {
        // Transition image from previous usage to transfer source layout
        InsertImageMemoryBarrier(commandBuffer,
                                 image,
                                 VK_ACCESS_SHADER_WRITE_BIT,
                                 VK_ACCESS_SHADER_READ_BIT,
                                 layout,
                                 layout,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                 mipLevels,
                                 0,
                                 arrayLayers,
                                 0);
    }
}

void ImageCube::ImageCubePipelineBarrierComputeToGraphic(const CommandBuffer& commandBuffer, std::optional<int> mipLevel) const
{
    if (mipLevel) {
        // Transition image from previous usage to transfer source layout
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
                                 mipLevel.value(),
                                 arrayLayers,
                                 0);
    }
    else {
        // Transition image from previous usage to transfer source layout
        InsertImageMemoryBarrier(commandBuffer,
                                 image,
                                 VK_ACCESS_SHADER_WRITE_BIT,
                                 VK_ACCESS_SHADER_READ_BIT,
                                 layout,
                                 layout,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                 mipLevels,
                                 0,
                                 arrayLayers,
                                 0);
    }
}

void ImageCube::ImageCubePipelineBarrierGraphicToCompute(const CommandBuffer& commandBuffer, std::optional<int> mipLevel) const
{
    if (mipLevel) {
        // Transition image from previous usage to transfer source layout
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
                                 mipLevel.value(),
                                 arrayLayers,
                                 0);
    }
    else {
        // Transition image from previous usage to transfer source layout
        InsertImageMemoryBarrier(commandBuffer,
                                 image,
                                 VK_ACCESS_SHADER_WRITE_BIT,
                                 VK_ACCESS_SHADER_READ_BIT,
                                 layout,
                                 layout,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                 mipLevels,
                                 0,
                                 arrayLayers,
                                 0);
    }
}

void ImageCube::CopyImage2DToImageCube(const CommandBuffer& commandBuffer, const Image* image, int mipLevel, int arrayLayer) const
{
    glm::uvec2 imageExtent = {image->GetExtent().width >> mipLevel, image->GetExtent().height >> mipLevel};
    glm::uvec2 layerExtent = {this->extent.width >> mipLevel, this->extent.height >> mipLevel};

    assert(imageExtent == layerExtent && "ImageCube::CopyImage2DToImageCube: Image extent and layer extent do not match.");

    InsertImageMemoryBarrier(commandBuffer,
                             this->image,
                             0,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             this->layout,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             mipLevel,
                             1,
                             arrayLayer);

    InsertImageMemoryBarrier(commandBuffer,
                             image->GetImage(),
                             VK_ACCESS_MEMORY_READ_BIT,
                             VK_ACCESS_TRANSFER_READ_BIT,
                             image->GetLayout(),
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             0,
                             1,
                             0);

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.baseArrayLayer = 0;
    imageCopyRegion.srcSubresource.layerCount     = 1;
    imageCopyRegion.srcSubresource.mipLevel       = 0;
    imageCopyRegion.srcOffset                     = {0, 0, 0};
    imageCopyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.baseArrayLayer = arrayLayer;
    imageCopyRegion.dstSubresource.layerCount     = 1;
    imageCopyRegion.dstSubresource.mipLevel       = mipLevel;
    imageCopyRegion.dstOffset                     = {0, 0, 0};
    imageCopyRegion.extent.width                  = imageExtent.x;
    imageCopyRegion.extent.height                 = imageExtent.y;
    imageCopyRegion.extent.depth                  = 1;

    vkCmdCopyImage(commandBuffer.GetCommandBuffer(),
                   image->GetImage(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   this->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &imageCopyRegion);

    InsertImageMemoryBarrier(commandBuffer,
                             this->image,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             this->layout,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             mipLevel,
                             1,
                             arrayLayer);

    InsertImageMemoryBarrier(commandBuffer,
                             image->GetImage(),
                             VK_ACCESS_TRANSFER_READ_BIT,
                             VK_ACCESS_SHADER_READ_BIT,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             image->GetLayout(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             1,
                             0,
                             1,
                             0);
}

void ImageCube::ClearImageCube(const CommandBuffer& commandBuffer, const glm::vec4& color) const
{
    // TransitionImageLayout(image, format, layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);

    VkClearColorValue clearColor = {};
    std::memcpy(clearColor.float32, &color, sizeof(color));

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = mipLevels;
    subresourceRange.baseArrayLayer          = 0;
    subresourceRange.layerCount              = arrayLayers;

    vkCmdClearColorImage(commandBuffer.GetCommandBuffer(), image, layout, &clearColor, 1, &subresourceRange);

    // TransitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 0, arrayLayers, 0);
}

void ImageCube::ClearImageCube(const glm::vec4& color) const
{
    CommandBuffer commandBuffer;
    ClearImageCube(commandBuffer, color);

    commandBuffer.SubmitIdle();
}
}   // namespace MapleLeaf