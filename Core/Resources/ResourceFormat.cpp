#include "ResourceFormat.h"

namespace MapleLeaf {
const FormatDesc kFormatDesc[] =
    {
        // Format                           Name,           BytesPerBlock ChannelCount  Type          {bDepth,   bStencil, bCompressed},   {CompressionRatio.Width,     CompressionRatio.Height}    {numChannelBits.x, numChannelBits.y, numChannelBits.z, numChannelBits.w}
        {ResourceFormat::Unknown,            "Unknown",         0,              0,  FormatType::Unknown,    {false,  false, false,},        {1, 1},                                                  {0, 0, 0, 0    }},
        {ResourceFormat::R8Unorm,            "R8Unorm",         1,              1,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {8, 0, 0, 0    }},
        {ResourceFormat::R8Snorm,            "R8Snorm",         1,              1,  FormatType::Snorm,      {false,  false, false,},        {1, 1},                                                  {8, 0, 0, 0    }},
        {ResourceFormat::R16Unorm,           "R16Unorm",        2,              1,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {16, 0, 0, 0   }},
        {ResourceFormat::R16Snorm,           "R16Snorm",        2,              1,  FormatType::Snorm,      {false,  false, false,},        {1, 1},                                                  {16, 0, 0, 0   }},
        {ResourceFormat::RG8Unorm,           "RG8Unorm",        2,              2,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {8, 8, 0, 0    }},
        {ResourceFormat::RG8Snorm,           "RG8Snorm",        2,              2,  FormatType::Snorm,      {false,  false, false,},        {1, 1},                                                  {8, 8, 0, 0    }},
        {ResourceFormat::RG16Unorm,          "RG16Unorm",       4,              2,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {16, 16, 0, 0  }},
        {ResourceFormat::RG16Snorm,          "RG16Snorm",       4,              2,  FormatType::Snorm,      {false,  false, false,},        {1, 1},                                                  {16, 16, 0, 0  }},
        {ResourceFormat::RGB16Unorm,         "RGB16Unorm",      6,              3,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 0 }},
        {ResourceFormat::RGB16Snorm,         "RGB16Snorm",      6,              3,  FormatType::Snorm,      {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 0 }},
        {ResourceFormat::R24UnormX8,         "R24UnormX8",      4,              2,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {24, 8, 0, 0   }},
        {ResourceFormat::RGB5A1Unorm,        "RGB5A1Unorm",     2,              4,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {5, 5, 5, 1    }},
        {ResourceFormat::RGBA8Unorm,         "RGBA8Unorm",      4,              4,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::RGBA8Snorm,         "RGBA8Snorm",      4,              4,  FormatType::Snorm,      {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::RGB10A2Unorm,       "RGB10A2Unorm",    4,              4,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {10, 10, 10, 2 }},
        {ResourceFormat::RGB10A2Uint,        "RGB10A2Uint",     4,              4,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {10, 10, 10, 2 }},
        {ResourceFormat::RGBA16Unorm,        "RGBA16Unorm",     8,              4,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 16}},
        {ResourceFormat::RGBA8UnormSrgb,     "RGBA8UnormSrgb",  4,              4,  FormatType::UnormSrgb,  {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        // Format                           Name,           BytesPerBlock ChannelCount  Type          {bDepth,   bStencil, bCompressed},   {CompressionRatio.Width,     CompressionRatio.Height}
        {ResourceFormat::R16Float,           "R16Float",        2,              1,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {16, 0, 0, 0   }},
        {ResourceFormat::RG16Float,          "RG16Float",       4,              2,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {16, 16, 0, 0  }},
        {ResourceFormat::RGB16Float,         "RGB16Float",      6,              3,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 0 }},
        {ResourceFormat::RGBA16Float,        "RGBA16Float",     8,              4,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 16}},
        {ResourceFormat::R32Float,           "R32Float",        4,              1,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {32, 0, 0, 0   }},
        {ResourceFormat::R32FloatX32,        "R32FloatX32",     8,              2,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {32, 32, 0, 0  }},
        {ResourceFormat::RG32Float,          "RG32Float",       8,              2,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {32, 32, 0, 0  }},
        {ResourceFormat::RGB32Float,         "RGB32Float",      12,             3,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {32, 32, 32, 0 }},
        {ResourceFormat::RGBA32Float,        "RGBA32Float",     16,             4,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {32, 32, 32, 32}},
        {ResourceFormat::R11G11B10Float,     "R11G11B10Float",  4,              3,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {11, 11, 10, 0 }},
        {ResourceFormat::RGB9E5Float,        "RGB9E5Float",     4,              3,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {9, 9, 9, 5    }},
        {ResourceFormat::R8Int,              "R8Int",           1,              1,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {8, 0, 0, 0    }},
        {ResourceFormat::R8Uint,             "R8Uint",          1,              1,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {8, 0, 0, 0    }},
        {ResourceFormat::R16Int,             "R16Int",          2,              1,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {16, 0, 0, 0   }},
        {ResourceFormat::R16Uint,            "R16Uint",         2,              1,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {16, 0, 0, 0   }},
        {ResourceFormat::R32Int,             "R32Int",          4,              1,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {32, 0, 0, 0   }},
        {ResourceFormat::R32Uint,            "R32Uint",         4,              1,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {32, 0, 0, 0   }},
        {ResourceFormat::RG8Int,             "RG8Int",          2,              2,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {8, 8, 0, 0    }},
        {ResourceFormat::RG8Uint,            "RG8Uint",         2,              2,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {8, 8, 0, 0    }},
        {ResourceFormat::RG16Int,            "RG16Int",         4,              2,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {16, 16, 0, 0  }},
        {ResourceFormat::RG16Uint,           "RG16Uint",        4,              2,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {16, 16, 0, 0  }},
        {ResourceFormat::RG32Int,            "RG32Int",         8,              2,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {32, 32, 0, 0  }},
        {ResourceFormat::RG32Uint,           "RG32Uint",        8,              2,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {32, 32, 0, 0  }},
        // Format                           Name,           BytesPerBlock ChannelCount  Type          {bDepth,   bStencil, bCompressed},   {CompressionRatio.Width,     CompressionRatio.Height}
        {ResourceFormat::RGB16Int,           "RGB16Int",        6,              3,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 0 }},
        {ResourceFormat::RGB16Uint,          "RGB16Uint",       6,              3,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 0 }},
        {ResourceFormat::RGB32Int,           "RGB32Int",       12,              3,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {32, 32, 32, 0 }},
        {ResourceFormat::RGB32Uint,          "RGB32Uint",      12,              3,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {32, 32, 32, 0 }},
        {ResourceFormat::RGBA8Int,           "RGBA8Int",        4,              4,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::RGBA8Uint,          "RGBA8Uint",       4,              4,  FormatType::Uint,       {false, false, false, },        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::RGBA16Int,          "RGBA16Int",       8,              4,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 16}},
        {ResourceFormat::RGBA16Uint,         "RGBA16Uint",      8,              4,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {16, 16, 16, 16}},
        {ResourceFormat::RGBA32Int,          "RGBA32Int",      16,              4,  FormatType::Sint,       {false,  false, false,},        {1, 1},                                                  {32, 32, 32, 32}},
        {ResourceFormat::RGBA32Uint,         "RGBA32Uint",     16,              4,  FormatType::Uint,       {false,  false, false,},        {1, 1},                                                  {32, 32, 32, 32}},
        {ResourceFormat::BGRA8Unorm,         "BGRA8Unorm",      4,              4,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::BGRA8UnormSrgb,     "BGRA8UnormSrgb",  4,              4,  FormatType::UnormSrgb,  {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::BGRX8Unorm,         "BGRX8Unorm",      4,              4,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::BGRX8UnormSrgb,     "BGRX8UnormSrgb",  4,              4,  FormatType::UnormSrgb,  {false,  false, false,},        {1, 1},                                                  {8, 8, 8, 8    }},
        {ResourceFormat::Alpha8Unorm,        "Alpha8Unorm",     1,              1,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {0, 0, 0, 8    }},
        {ResourceFormat::Alpha32Float,       "Alpha32Float",    4,              1,  FormatType::Float,      {false,  false, false,},        {1, 1},                                                  {0, 0, 0, 32   }},
        // Format                           Name,           BytesPerBlock ChannelCount  Type          {bDepth,   bStencil, bCompressed},   {CompressionRatio.Width,     CompressionRatio.Height}
        {ResourceFormat::R5G6B5Unorm,        "R5G6B5Unorm",     2,              3,  FormatType::Unorm,      {false,  false, false,},        {1, 1},                                                  {5, 6, 5, 0    }},
        {ResourceFormat::D32Float,           "D32Float",        4,              1,  FormatType::Float,      {true,   false, false,},        {1, 1},                                                  {32, 0, 0, 0   }},
        {ResourceFormat::D16Unorm,           "D16Unorm",        2,              1,  FormatType::Unorm,      {true,   false, false,},        {1, 1},                                                  {16, 0, 0, 0   }},
        {ResourceFormat::D32FloatS8X24,      "D32FloatS8X24",   8,              2,  FormatType::Float,      {true,   true,  false,},        {1, 1},                                                  {32, 8, 24, 0  }},
        {ResourceFormat::D24UnormS8,         "D24UnormS8",      4,              2,  FormatType::Unorm,      {true,   true,  false,},        {1, 1},                                                  {24, 8, 0, 0   }},
        {ResourceFormat::BC1Unorm,           "BC1Unorm",        8,              3,  FormatType::Unorm,      {false,  false, true, },        {4, 4},                                                  {64, 0, 0, 0   }},
        {ResourceFormat::BC1UnormSrgb,       "BC1UnormSrgb",    8,              3,  FormatType::UnormSrgb,  {false,  false, true, },        {4, 4},                                                  {64, 0, 0, 0   }},
        {ResourceFormat::BC2Unorm,           "BC2Unorm",        16,             4,  FormatType::Unorm,      {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC2UnormSrgb,       "BC2UnormSrgb",    16,             4,  FormatType::UnormSrgb,  {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC3Unorm,           "BC3Unorm",        16,             4,  FormatType::Unorm,      {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC3UnormSrgb,       "BC3UnormSrgb",    16,             4,  FormatType::UnormSrgb,  {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC4Unorm,           "BC4Unorm",        8,              1,  FormatType::Unorm,      {false,  false, true, },        {4, 4},                                                  {64, 0, 0, 0   }},
        {ResourceFormat::BC4Snorm,           "BC4Snorm",        8,              1,  FormatType::Snorm,      {false,  false, true, },        {4, 4},                                                  {64, 0, 0, 0   }},
        {ResourceFormat::BC5Unorm,           "BC5Unorm",        16,             2,  FormatType::Unorm,      {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC5Snorm,           "BC5Snorm",        16,             2,  FormatType::Snorm,      {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},

        {ResourceFormat::BC6HS16,            "BC6HS16",         16,             3,  FormatType::Float,      {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC6HU16,            "BC6HU16",         16,             3,  FormatType::Float,      {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC7Unorm,           "BC7Unorm",        16,             4,  FormatType::Unorm,      {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
        {ResourceFormat::BC7UnormSrgb,       "BC7UnormSrgb",    16,             4,  FormatType::UnormSrgb,  {false,  false, true, },        {4, 4},                                                  {128, 0, 0, 0  }},
    };

    struct VulkanFormatDesc 
    {
        ResourceFormat format;
        VkFormat       vkFormat;
    };

    const VulkanFormatDesc kVulkanFormatDesc[] = 
    {
        {ResourceFormat::Unknown,                       VK_FORMAT_UNDEFINED},
        {ResourceFormat::R8Unorm,           VK_FORMAT_R8_UNORM},
        {ResourceFormat::R8Snorm,           VK_FORMAT_R8_SNORM},
        {ResourceFormat::R16Unorm,          VK_FORMAT_R16_UNORM},
        {ResourceFormat::R16Snorm,          VK_FORMAT_R16_SNORM},
        {ResourceFormat::RG8Unorm,          VK_FORMAT_R8G8_UNORM},
        {ResourceFormat::RG8Snorm,          VK_FORMAT_R8G8_SNORM},
        {ResourceFormat::RG16Unorm,         VK_FORMAT_R16G16_UNORM},
        {ResourceFormat::RG16Snorm,         VK_FORMAT_R16G16_SNORM},
        {ResourceFormat::RGB16Unorm,        VK_FORMAT_R16G16B16_UNORM},
        {ResourceFormat::RGB16Snorm,        VK_FORMAT_R16G16B16_SNORM},
        {ResourceFormat::R24UnormX8,        VK_FORMAT_D24_UNORM_S8_UINT},
        {ResourceFormat::RGB5A1Unorm,       VK_FORMAT_B5G5R5A1_UNORM_PACK16},
        {ResourceFormat::RGBA8Unorm,        VK_FORMAT_R8G8B8A8_UNORM},
        {ResourceFormat::RGBA8Snorm,        VK_FORMAT_R8G8B8A8_SNORM},
        {ResourceFormat::RGB10A2Unorm,      VK_FORMAT_A2B10G10R10_UNORM_PACK32},
        {ResourceFormat::RGB10A2Uint,       VK_FORMAT_A2B10G10R10_UINT_PACK32},
        {ResourceFormat::RGBA16Unorm,       VK_FORMAT_R16G16B16A16_UNORM},
        {ResourceFormat::RGBA8UnormSrgb,    VK_FORMAT_R8G8B8A8_SRGB},
        {ResourceFormat::R16Float,          VK_FORMAT_R16_SFLOAT},
        {ResourceFormat::RG16Float,         VK_FORMAT_R16G16_SFLOAT},
        {ResourceFormat::RGB16Float,        VK_FORMAT_R16G16B16_SFLOAT},
        {ResourceFormat::RGBA16Float,       VK_FORMAT_R16G16B16A16_SFLOAT},
        {ResourceFormat::R32Float,          VK_FORMAT_R32_SFLOAT},
        {ResourceFormat::R32FloatX32,       VK_FORMAT_R32G32_SFLOAT},
        {ResourceFormat::RG32Float,         VK_FORMAT_R32G32_SFLOAT},
        {ResourceFormat::RGB32Float,        VK_FORMAT_R32G32B32_SFLOAT},
        {ResourceFormat::RGBA32Float,       VK_FORMAT_R32G32B32A32_SFLOAT},
        {ResourceFormat::R11G11B10Float,    VK_FORMAT_B10G11R11_UFLOAT_PACK32},
        {ResourceFormat::RGB9E5Float,       VK_FORMAT_E5B9G9R9_UFLOAT_PACK32},
        {ResourceFormat::R8Int,             VK_FORMAT_R8_SINT},
        {ResourceFormat::R8Uint,            VK_FORMAT_R8_UINT},
        {ResourceFormat::R16Int,            VK_FORMAT_R16_SINT},
        {ResourceFormat::R16Uint,           VK_FORMAT_R16_UINT},
        {ResourceFormat::R32Int,            VK_FORMAT_R32_SINT},
        {ResourceFormat::R32Uint,           VK_FORMAT_R32_UINT},
        {ResourceFormat::RG8Int,            VK_FORMAT_R8G8_SINT},
        {ResourceFormat::RG8Uint,           VK_FORMAT_R8G8_UINT},
        {ResourceFormat::RG16Int,           VK_FORMAT_R16G16_SINT},
        {ResourceFormat::RG16Uint,          VK_FORMAT_R16G16_UINT},
        {ResourceFormat::RG32Int,           VK_FORMAT_R32G32_SINT},
        {ResourceFormat::RG32Uint,          VK_FORMAT_R32G32_UINT},
        {ResourceFormat::RGB16Int,          VK_FORMAT_R16G16B16_SINT},
        {ResourceFormat::RGB16Uint,         VK_FORMAT_R16G16B16_UINT},
        {ResourceFormat::RGB32Int,          VK_FORMAT_R32G32B32_SINT},
        {ResourceFormat::RGB32Uint,         VK_FORMAT_R32G32B32_UINT},
        {ResourceFormat::RGBA8Int,          VK_FORMAT_R8G8B8A8_SINT},
        {ResourceFormat::RGBA8Uint,         VK_FORMAT_R8G8B8A8_UINT},
        {ResourceFormat::RGBA16Int,         VK_FORMAT_R16G16B16A16_SINT},
        {ResourceFormat::RGBA16Uint,        VK_FORMAT_R16G16B16A16_UINT},
        {ResourceFormat::RGBA32Int,         VK_FORMAT_R32G32B32A32_SINT},
        {ResourceFormat::RGBA32Uint,        VK_FORMAT_R32G32B32A32_UINT},
        {ResourceFormat::BGRA8Unorm,        VK_FORMAT_B8G8R8A8_UNORM},
        {ResourceFormat::BGRA8UnormSrgb,    VK_FORMAT_B8G8R8A8_SRGB},
        {ResourceFormat::BGRX8Unorm,        VK_FORMAT_B8G8R8A8_UNORM},
        {ResourceFormat::BGRX8UnormSrgb,    VK_FORMAT_B8G8R8A8_SRGB},
        {ResourceFormat::Alpha8Unorm,       VK_FORMAT_A8B8G8R8_UNORM_PACK32},
        {ResourceFormat::Alpha32Float,      VK_FORMAT_R32_SFLOAT},
        {ResourceFormat::R5G6B5Unorm,       VK_FORMAT_B5G6R5_UNORM_PACK16},
        {ResourceFormat::D32Float,          VK_FORMAT_D32_SFLOAT},
        {ResourceFormat::D16Unorm,          VK_FORMAT_D16_UNORM},
        {ResourceFormat::D32FloatS8X24,     VK_FORMAT_D32_SFLOAT_S8_UINT},
        {ResourceFormat::D24UnormS8,        VK_FORMAT_D24_UNORM_S8_UINT},
        {ResourceFormat::BC1Unorm,          VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
        {ResourceFormat::BC1UnormSrgb,      VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
        {ResourceFormat::BC2Unorm,          VK_FORMAT_BC2_UNORM_BLOCK},
        {ResourceFormat::BC2UnormSrgb,      VK_FORMAT_BC2_SRGB_BLOCK},
        {ResourceFormat::BC3Unorm,          VK_FORMAT_BC3_UNORM_BLOCK},
        {ResourceFormat::BC3UnormSrgb,      VK_FORMAT_BC3_SRGB_BLOCK},
        {ResourceFormat::BC4Unorm,          VK_FORMAT_BC4_UNORM_BLOCK},
        {ResourceFormat::BC4Snorm,          VK_FORMAT_BC4_SNORM_BLOCK},
        {ResourceFormat::BC5Unorm,          VK_FORMAT_BC5_UNORM_BLOCK},
        {ResourceFormat::BC5Snorm,          VK_FORMAT_BC5_SNORM_BLOCK},
        {ResourceFormat::BC6HS16,           VK_FORMAT_BC6H_SFLOAT_BLOCK},
        {ResourceFormat::BC6HU16,           VK_FORMAT_BC6H_UFLOAT_BLOCK},
        {ResourceFormat::BC7Unorm,          VK_FORMAT_BC7_UNORM_BLOCK},
        {ResourceFormat::BC7UnormSrgb,      VK_FORMAT_BC7_SRGB_BLOCK},
    };

    VkFormat GetVulkanFormat(ResourceFormat format)
    {
        assert(kVulkanFormatDesc[static_cast<uint32_t>(format)].format == format);
        return kVulkanFormatDesc[static_cast<uint32_t>(format)].vkFormat;
    }

    ResourceFormat GetResourceFormat(VkFormat format)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(ResourceFormat::Count); ++i)
        {
            if (kVulkanFormatDesc[i].vkFormat == format)
            {
                return kVulkanFormatDesc[i].format;
            }
        }

        return ResourceFormat::Unknown;
    }
}