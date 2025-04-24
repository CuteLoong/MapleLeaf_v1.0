#pragma once
#include "Resource.hpp"
#include "volk.h"
#include <cassert>
#include <string>

namespace MapleLeaf {

enum class TextureChannelFlags : uint32_t
{
    None  = 0x0,
    Red   = 0x1,
    Green = 0x2,
    Blue  = 0x4,
    Alpha = 0x8,
    RGB   = 0x7,
    RGBA  = 0xf,
};

enum class ResourceFormat : uint32_t
{
    Unknown,
    R8Unorm,
    R8Snorm,
    R16Unorm,
    R16Snorm,
    RG8Unorm,
    RG8Snorm,
    RG16Unorm,
    RG16Snorm,
    RGB16Unorm,
    RGB16Snorm,
    R24UnormX8,
    RGB5A1Unorm,
    RGBA8Unorm,
    RGBA8Snorm,
    RGB10A2Unorm,
    RGB10A2Uint,
    RGBA16Unorm,
    RGBA8UnormSrgb,
    R16Float,
    RG16Float,
    RGB16Float,
    RGBA16Float,
    R32Float,
    R32FloatX32,
    RG32Float,
    RGB32Float,
    RGBA32Float,
    R11G11B10Float,
    RGB9E5Float,
    R8Int,
    R8Uint,
    R16Int,
    R16Uint,
    R32Int,
    R32Uint,
    RG8Int,
    RG8Uint,
    RG16Int,
    RG16Uint,
    RG32Int,
    RG32Uint,
    RGB16Int,
    RGB16Uint,
    RGB32Int,
    RGB32Uint,
    RGBA8Int,
    RGBA8Uint,
    RGBA16Int,
    RGBA16Uint,
    RGBA32Int,
    RGBA32Uint,

    BGRA8Unorm,
    BGRA8UnormSrgb,

    BGRX8Unorm,
    BGRX8UnormSrgb,
    Alpha8Unorm,
    Alpha32Float,
    R5G6B5Unorm,

    // Depth-stencil
    D32Float,
    D16Unorm,
    D32FloatS8X24,
    D24UnormS8,

    // Compressed formats
    BC1Unorm,   // DXT1
    BC1UnormSrgb,
    BC2Unorm,   // DXT3
    BC2UnormSrgb,
    BC3Unorm,   // DXT5
    BC3UnormSrgb,
    BC4Unorm,   // RGTC Unsigned Red
    BC4Snorm,   // RGTC Signed Red
    BC5Unorm,   // RGTC Unsigned RG
    BC5Snorm,   // RGTC Signed RG
    BC6HS16,
    BC6HU16,
    BC7Unorm,
    BC7UnormSrgb,

    Count
};

enum class FormatType
{
    Unknown,     ///< Unknown format Type
    Float,       ///< Floating-point formats
    Unorm,       ///< Unsigned normalized formats
    UnormSrgb,   ///< Unsigned normalized SRGB formats
    Snorm,       ///< Signed normalized formats
    Uint,        ///< Unsigned integer formats
    Sint         ///< Signed integer formats
};

struct FormatDesc
{
    ResourceFormat    format;
    const std::string name;
    uint32_t          bytesPerBlock;
    uint32_t          channelCount;
    FormatType        Type;
    struct
    {
        bool isDepth;
        bool isStencil;
        bool isCompressed;
    };
    struct
    {
        uint32_t width;
        uint32_t height;
    } compressionRatio;
    int numChannelBits[4];
};

extern const FormatDesc kFormatDesc[];

inline uint32_t GetFormatChannelCount(ResourceFormat format)
{
    assert(kFormatDesc[(uint32_t)format].format == format);
    return kFormatDesc[(uint32_t)format].channelCount;
}

inline uint32_t GetFormatWidthCompressionRatio(ResourceFormat format)
{
    assert(kFormatDesc[(uint32_t)format].format == format);
    return kFormatDesc[(uint32_t)format].compressionRatio.width;
}

inline uint32_t GetFormatBytesPerBlock(ResourceFormat format, uint32_t width)
{
    assert(width % GetFormatWidthCompressionRatio(format) == 0);
    return width * kFormatDesc[static_cast<uint32_t>(format)].bytesPerBlock;
}

/** Get number of bits used for a given color channel.
 */
inline uint32_t GetNumChannelBits(ResourceFormat format, int channel)
{
    return kFormatDesc[(uint32_t)format].numChannelBits[channel];
}

/** Get the format Type
 */
inline FormatType GetFormatType(ResourceFormat format)
{
    assert(kFormatDesc[(uint32_t)format].format == format);
    return kFormatDesc[(uint32_t)format].Type;
}

/** Get the number of bytes per format
 */
inline uint32_t GetFormatBytesPerBlock(ResourceFormat format)
{
    assert(kFormatDesc[(uint32_t)format].format == format);
    return kFormatDesc[(uint32_t)format].bytesPerBlock;
}

/**
 * @brief Get the Vulkan Format object
 *
 * @param format
 * @return VkFormat
 */
VkFormat GetVulkanFormat(ResourceFormat format);

/**
 * @brief Get the Resource Format object
 *
 * @param format
 * @return ResourceFormat
 */
ResourceFormat GetResourceFormat(VkFormat format);

}   // namespace MapleLeaf