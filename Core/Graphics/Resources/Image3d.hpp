#pragma once

#include "Bitmap.hpp"
#include "Image.hpp"
#include <filesystem>


namespace MapleLeaf {
class Image3d : public Image
{
public:
    /**
     * Creates a new 3D image.
     * @param extent The images extent in pixels.
     * @param format The format and type of the texel blocks that will be contained in the image.
     * @param layout The layout that the image subresources accessible from.
     * @param usage The intended usage of the image.
     * @param filter The magnification/minification filter to apply to lookups.
     * @param addressMode The addressing mode for outside [0..1] range.
     * @param samples The number of samples per texel.
     * @param anisotropic If anisotropic filtering is enabled.
     * @param mipmap If mapmaps will be generated.
     */
    explicit Image3d(const glm::uvec3& extent, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                     VkImageLayout     layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VkFilter filter = VK_FILTER_LINEAR,
                     VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                     bool anisotropic = false, bool mipmap = false);


    // void CopyImage3d(const CommandBuffer& commandBuffer, const Image3d& image2d, int mipLevel = 0) const;
    void ClearImage3d(const glm::vec4& color) const;

    void Image3dPipelineBarrierComputeToCompute(const CommandBuffer& commandBuffer, int mipLevel = 0) const;
    void Image3dPipelineBarrierComputeToGraphic(const CommandBuffer& commandBuffer, int mipLevel = 0) const;
    void Image3dPipelineBarrierGraphicToCompute(const CommandBuffer& commandBuffer, int mipLevel = 0) const;

private:
    void Load();

    std::filesystem::path filename;

    bool     anisotropic;
    bool     mipmap;
    uint32_t components = 0;
};
}   // namespace MapleLeaf