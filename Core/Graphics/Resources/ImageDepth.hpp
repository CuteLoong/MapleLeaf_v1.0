#pragma once

#include "Image.hpp"

namespace MapleLeaf {
class ImageDepth : public Image
{
public:
    explicit ImageDepth(const glm::uvec2& extent, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

    void CopyImageDepth(const CommandBuffer& commandBuffer, const ImageDepth& imageDepth) const;

    void ImageDepthPipelineBarrierGraphicToCompute(const CommandBuffer& commandBuffer, int mipLevel = 0) const;
};
}   // namespace MapleLeaf