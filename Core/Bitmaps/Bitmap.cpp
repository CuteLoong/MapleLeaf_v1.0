#include "Bitmap.hpp"
#include "Files.hpp"
#include "Log.hpp"
#include "ResourceFormat.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stb_image_write.h>

#include "FreeImage.h"

namespace {
static bool isConvertibleToRGBA32Float(MapleLeaf::ResourceFormat format)
{
    MapleLeaf::FormatType type         = GetFormatType(format);
    bool                  isHalfFormat = (type == MapleLeaf::FormatType::Float && GetNumChannelBits(format, 0) == 16);
    bool isLargeIntFormat = ((type == MapleLeaf::FormatType::Uint || type == MapleLeaf::FormatType::Sint) && GetNumChannelBits(format, 0) >= 16);
    return isHalfFormat || isLargeIntFormat;
}

/** Converts 96bpp to 128bpp RGBA without clamping.
Note that we can't use FreeImage_ConvertToRGBAF() as it clamps to [0,1].
*/
static FIBITMAP* convertToRGBAF(FIBITMAP* pDib)
{
    const unsigned width  = FreeImage_GetWidth(pDib);
    const unsigned height = FreeImage_GetHeight(pDib);

    auto pNew = FreeImage_AllocateT(FIT_RGBAF, width, height);
    FreeImage_CloneMetadata(pNew, pDib);

    const unsigned src_pitch = FreeImage_GetPitch(pDib);
    const unsigned dst_pitch = FreeImage_GetPitch(pNew);

    const BYTE* src_bits = (BYTE*)FreeImage_GetBits(pDib);
    BYTE*       dst_bits = (BYTE*)FreeImage_GetBits(pNew);

    for (unsigned y = 0; y < height; y++) {
        const FIRGBF* src_pixel = (FIRGBF*)src_bits;
        FIRGBAF*      dst_pixel = (FIRGBAF*)dst_bits;

        for (unsigned x = 0; x < width; x++) {
            // Convert pixels directly, while adding a "dummy" alpha of 1.0
            dst_pixel[x].red   = src_pixel[x].red;
            dst_pixel[x].green = src_pixel[x].green;
            dst_pixel[x].blue  = src_pixel[x].blue;
            dst_pixel[x].alpha = 1.0F;
        }
        src_bits += src_pitch;
        dst_bits += dst_pitch;
    }
    return pNew;
}

template<typename SrcT>
static std::vector<float> convertIntToRGBA32Float(uint32_t width, uint32_t height, uint32_t channelCount, const void* pData)
{
    std::vector<float> newData(width * height * 4u, 0.f);
    const SrcT*        pSrc = reinterpret_cast<const SrcT*>(pData);
    float*             pDst = newData.data();

    for (uint32_t i = 0; i < width * height; ++i) {
        for (uint32_t c = 0; c < channelCount; ++c) {
            *pDst++ = float(*pSrc++) / float(std::numeric_limits<SrcT>::max());
        }
        pDst += (4 - channelCount);
    }

    return newData;
}

/** Converts half float image to RGBA float image.
 */
static std::vector<float> convertHalfToRGBA32Float(uint32_t width, uint32_t height, uint32_t channelCount, const void* pData)
{
    std::vector<float>        newData(width * height * 4u, 0.f);
    const glm::detail::hdata* pSrc = reinterpret_cast<const glm::detail::hdata*>(pData);
    float*                    pDst = newData.data();

    for (uint32_t i = 0; i < width * height; ++i) {
        for (uint32_t c = 0; c < channelCount; ++c) {
            *pDst++ = glm::detail::toFloat32(*pSrc++);
        }
        pDst += (4 - channelCount);
    }

    return newData;
}

/** Converts an image of the given format to an RGBA float image.
 */
static std::vector<float> convertToRGBA32Float(MapleLeaf::ResourceFormat format, uint32_t width, uint32_t height, const void* pData)
{
    assert(isConvertibleToRGBA32Float(format));

    MapleLeaf::FormatType type         = GetFormatType(format);
    uint32_t              channelCount = GetFormatChannelCount(format);
    uint32_t              channelBits  = GetNumChannelBits(format, 0);

    std::vector<float> floatData;

    if (type == MapleLeaf::FormatType::Float && channelBits == 16) {
        floatData = convertHalfToRGBA32Float(width, height, channelCount, pData);
    }
    else if (type == MapleLeaf::FormatType::Uint && channelBits == 16) {
        floatData = convertIntToRGBA32Float<uint16_t>(width, height, channelCount, pData);
    }
    else if (type == MapleLeaf::FormatType::Uint && channelBits == 32) {
        floatData = convertIntToRGBA32Float<uint32_t>(width, height, channelCount, pData);
    }
    else if (type == MapleLeaf::FormatType::Sint && channelBits == 16) {
        floatData = convertIntToRGBA32Float<int16_t>(width, height, channelCount, pData);
    }
    else if (type == MapleLeaf::FormatType::Sint && channelBits == 32) {
        floatData = convertIntToRGBA32Float<int32_t>(width, height, channelCount, pData);
    }
    else {
        assert(false);
    }

    // Default alpha channel to 1.
    if (channelCount < 4) {
        for (uint32_t i = 0; i < width * height; ++i) floatData[i * 4 + 3] = 1.f;
    }

    return floatData;
}

static FREE_IMAGE_FORMAT toFreeImageFormat(MapleLeaf::Bitmap::FileFormat fmt)
{
    switch (fmt) {
    case MapleLeaf::Bitmap::FileFormat::PngFile: return FIF_PNG;
    case MapleLeaf::Bitmap::FileFormat::JpegFile: return FIF_JPEG;
    case MapleLeaf::Bitmap::FileFormat::TgaFile: return FIF_TARGA;
    case MapleLeaf::Bitmap::FileFormat::BmpFile: return FIF_BMP;
    case MapleLeaf::Bitmap::FileFormat::PfmFile: return FIF_PFM;
    case MapleLeaf::Bitmap::FileFormat::ExrFile: return FIF_EXR;
    default: assert(false);
    }
    return FIF_PNG;
}
}   // namespace

namespace MapleLeaf {
Bitmap::Bitmap(std::filesystem::path filename)
    : filename(std::move(filename))
{
    Load(this->filename);
}

Bitmap::Bitmap(const glm::uvec2& size, ResourceFormat format)
    : size(size)
    , format(format)
    , rowPitch(GetFormatBytesPerBlock(format, size.x))
    , componentCount(GetFormatChannelCount(format))
{
    data = std::make_unique<uint8_t[]>(size.y * rowPitch);
}

Bitmap::Bitmap(std::unique_ptr<uint8_t[]>&& data, const glm::uvec2& size, ResourceFormat format)
    : data(std::move(data))
    , size(size)
    , format(format)
    , rowPitch(GetFormatBytesPerBlock(format, size.x))
    , componentCount(GetFormatChannelCount(format))
{}

void Bitmap::Load(const std::filesystem::path& filename)
{
    auto pathStr = filename.string();

    auto exisitPath = Files::Get()->GetExistPath(pathStr);
    if (!exisitPath) {
        Log::Error("Error when loading image file. Can't find image file: ", filename, '\n');
        return;
    }
    std::string imagePath = exisitPath->string();
    std::replace(imagePath.begin(), imagePath.end(), '\\', '/');

    FREE_IMAGE_FORMAT fifFormat = FIF_UNKNOWN;
    fifFormat                   = FreeImage_GetFileType(imagePath.c_str(), 0);
    if (fifFormat == FIF_UNKNOWN) {
        fifFormat = FreeImage_GetFIFFromFilename(imagePath.c_str());
        if (fifFormat == FIF_UNKNOWN) {
            Log::Error("Image type unknown: ", imagePath.c_str(), '\n');
            return;
        }
    }

    if (FreeImage_FIFSupportsReading(fifFormat) == false) {
        Log::Error("Library doesn't support the file format: ", imagePath.c_str(), '\n');
        return;
    }

    FIBITMAP* pDib = FreeImage_Load(fifFormat, imagePath.c_str());
    if (pDib == nullptr) {
        Log::Error("Can't read image file: ", imagePath.c_str(), '\n');
        return;
    }

    size = {static_cast<uint32_t>(FreeImage_GetWidth(pDib)), static_cast<uint32_t>(FreeImage_GetHeight(pDib))};

    if (size.x == 0 || size.y == 0 || FreeImage_GetBPP(pDib) == 0) {
        Log::Error("Invaild image: ", imagePath.c_str(), '\n');
        return;
    }

    FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(pDib);
    if (colorType == FIC_PALETTE) {
        auto pNew = FreeImage_ConvertTo32Bits(pDib);
        FreeImage_Unload(pDib);
        pDib = pNew;

        if (pDib == nullptr) {
            Log::Error("Failed to convert palettized image to RGBA format: ", imagePath.c_str(), '\n');
            return;
        }
    }

    format       = ResourceFormat::Unknown;
    uint32_t bpp = FreeImage_GetBPP(pDib);

    switch (bpp) {
    case 128: format = ResourceFormat::RGBA32Float; break;
    case 96: format = ResourceFormat::RGBA32Float; break;   // RGB32Float not supported
    case 64: format = ResourceFormat::RGBA16Float; break;
    // case 48: format = ResourceFormat::RGBA16Float; break;   // RGB16Float not supported
    case 32: format = ResourceFormat::BGRA8Unorm; break;
    case 24: format = ResourceFormat::BGRX8Unorm; break;
    case 16: format = FreeImage_GetImageType(pDib) == FIT_UINT16 ? ResourceFormat::R16Unorm : ResourceFormat::RG8Unorm; break;
    case 8: format = ResourceFormat::R8Unorm; break;
    default: Log::Error("Unsupported image format: ", exisitPath->string().c_str(), '\n'); break;
    }
    rowPitch       = GetFormatBytesPerBlock(format, size.x);
    componentCount = GetFormatChannelCount(format);

    if (bpp == 24) {
        bpp       = 32;
        auto pNew = FreeImage_ConvertTo32Bits(pDib);
        FreeImage_Unload(pDib);
        pDib = pNew;
    }
    else if (bpp == 96) {
        bpp       = 128;
        auto pNew = convertToRGBAF(pDib);
        FreeImage_Unload(pDib);
        pDib = pNew;
    }

    data = std::make_unique<uint8_t[]>(size.y * rowPitch);
    FreeImage_ConvertToRawBits(data.get(), pDib, rowPitch, bpp, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
    FreeImage_Unload(pDib);
}

void Bitmap::ConvertBGRAtoRGBA(uint8_t* bgraData, uint32_t width, uint32_t height) const
{
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t i = (y * width + x) * 4;
            std::swap(bgraData[i], bgraData[i + 2]);
        }
    }
}

void Bitmap::GammaCorrect(float gamma)
{
    for (uint32_t y = 0; y < size.y; ++y) {
        for (uint32_t x = 0; x < size.x; ++x) {
            uint32_t i = (y * size.x + x) * componentCount;
            for (uint32_t j = 0; j < componentCount; ++j) {
                if (j >= 3) continue;
                data[i + j] = static_cast<uint8_t>(255.0f * std::pow(data[i + j] / 255.0f, 1.0f / gamma));
            }
        }
    }
}

void Bitmap::Write(const std::filesystem::path& filename) const
{
    if (auto parentPath = filename.parent_path(); !parentPath.empty()) std::filesystem::create_directories(parentPath);

    std::ofstream os(filename, std::ios::binary | std::ios::out);
    int32_t       len;

    ConvertBGRAtoRGBA(data.get(), size.x, size.y);
    std::unique_ptr<uint8_t[]> png(stbi_write_png_to_mem(data.get(), size.x * componentCount, size.x, size.y, componentCount, &len));
    os.write(reinterpret_cast<char*>(png.get()), len);
}

void Bitmap::SaveImage(const std::filesystem::path& path, uint32_t width, uint32_t height, FileFormat fileFormat, ExportFlags exportFlags,
                       ResourceFormat resourceFormat, bool isTopDown, void* pData)
{
    if (auto parentPath = path.parent_path(); !parentPath.empty()) std::filesystem::create_directories(parentPath);

    if (pData == nullptr) {
        Log::Error("Bitmap::SaveImage provided no data to save.", '\n');
        return;
    }

    if (is_set(exportFlags, ExportFlags::Uncompressed) && is_set(exportFlags, ExportFlags::Lossy)) {
        Log::Error("Bitmap::SaveImage incompatible flags: lossy cannot be combined with uncompressed.", '\n');
        return;
    }

    if (fileFormat == FileFormat::DdsFile) {
        Log::Error("Bitmap::SaveImage DDS format not supported.", '\n');
        return;
    }

    int       flags         = 0;
    FIBITMAP* pImage        = nullptr;
    uint32_t  bytesPerPixel = GetFormatBytesPerBlock(resourceFormat);
    uint32_t  channelCount  = GetFormatChannelCount(resourceFormat);

    if (resourceFormat == ResourceFormat::RGBA8Unorm || resourceFormat == ResourceFormat::RGBA8Snorm ||
        resourceFormat == ResourceFormat::RGBA8UnormSrgb) {
        for (uint32_t a = 0; a < width * height; a++) {
            uint32_t* pPixel = (uint32_t*)pData;
            pPixel += a;
            uint8_t* ch = (uint8_t*)pPixel;
            std::swap(ch[0], ch[2]);
            if (is_set(exportFlags, ExportFlags::ExportAlpha) == false) {
                ch[3] = 0xff;
            }
        }
    }

    if (fileFormat == Bitmap::FileFormat::ExrFile || fileFormat == Bitmap::FileFormat::PfmFile) {
        std::vector<float> floatData;
        if (isConvertibleToRGBA32Float(resourceFormat)) {
            floatData      = convertToRGBA32Float(resourceFormat, width, height, pData);
            pData          = floatData.data();
            resourceFormat = ResourceFormat::RGBA32Float;
            bytesPerPixel  = 16;
        }
        else if (resourceFormat != ResourceFormat::R32Float && resourceFormat != ResourceFormat::R32FloatX32 && bytesPerPixel != 16 &&
                 bytesPerPixel != 12) {
            Log::Error("Bitmap::saveImage supports only 32-bit/channel RGB/RGBA or 16-bit RGBA images as PFM/EXR files.", '\n');
            return;
        }

        const bool exportAlpha = is_set(exportFlags, ExportFlags::ExportAlpha);


        if (fileFormat == Bitmap::FileFormat::PfmFile) {
            if (is_set(exportFlags, ExportFlags::Lossy)) {
                Log::Error("Bitmap::saveImage PFM format does not support lossy compression.", '\n');
                return;
            }
            if (exportAlpha) {
                Log::Error("Bitmap::saveImage PFM format does not support alpha channel.", '\n');
                return;
            }
        }

        if (exportAlpha && bytesPerPixel != 16) {
            Log::Error("Bitmap::saveImage PFM format does not support alpha channel.", '\n');
            return;
        }

        if (resourceFormat == ResourceFormat::R32Float) {
            pImage     = FreeImage_AllocateT(FIT_FLOAT, width, height);
            BYTE* head = (BYTE*)pData;
            for (unsigned y = 0; y < height; y++) {
                float* dstBits = (float*)FreeImage_GetScanLine(pImage, height - y - 1);
                // 每行复制 width 个浮点数
                std::memcpy(dstBits, head, width * sizeof(float));
                head += width * sizeof(float);
            }
        }
        else if (resourceFormat == ResourceFormat::R32FloatX32) {
            pImage     = FreeImage_AllocateT(FIT_RGBF, width, height);
            BYTE* head = (BYTE*)pData;
            for (unsigned y = 0; y < height; y++) {
                float* dstBits = (float*)FreeImage_GetScanLine(pImage, height - y - 1);
                for (unsigned x = 0; x < width; x++) {
                    const float* src   = (const float*)head + x * 2;
                    dstBits[x * 3 + 0] = src[0];   // R 分量
                    dstBits[x * 3 + 1] = src[1];   // G 分量
                    dstBits[x * 3 + 2] = 0.0f;     // B 分量置零
                }
                head += width * 2 * sizeof(float);
            }
        }
        else {
            bool scanlineCopy = exportAlpha ? bytesPerPixel == 16 : bytesPerPixel == 12;

            pImage     = FreeImage_AllocateT(exportAlpha ? FIT_RGBAF : FIT_RGBF, width, height);
            BYTE* head = (BYTE*)pData;
            for (unsigned y = 0; y < height; y++) {
                float* dstBits = (float*)FreeImage_GetScanLine(pImage, height - y - 1);
                if (scanlineCopy) {
                    std::memcpy(dstBits, head, bytesPerPixel * width);
                }
                else {
                    assert(exportAlpha == false);
                    for (unsigned x = 0; x < width; x++) {
                        dstBits[x * 3 + 0] = (((float*)head)[x * 4 + 0]);
                        dstBits[x * 3 + 1] = (((float*)head)[x * 4 + 1]);
                        dstBits[x * 3 + 2] = (((float*)head)[x * 4 + 2]);
                    }
                }
                head += bytesPerPixel * width;
            }
        }

        if (fileFormat == Bitmap::FileFormat::ExrFile) {
            flags |= EXR_FLOAT;
            if (is_set(exportFlags, ExportFlags::Uncompressed)) {
                flags |= EXR_NONE | EXR_FLOAT;
            }
            else if (is_set(exportFlags, ExportFlags::Lossy)) {
                flags |= EXR_B44 | EXR_ZIP;
            }
        }
    }
    else {
        FIBITMAP* pTemp = FreeImage_ConvertFromRawBits((BYTE*)pData,
                                                       width,
                                                       height,
                                                       bytesPerPixel * width,
                                                       bytesPerPixel * 8,
                                                       FI_RGBA_RED_MASK,
                                                       FI_RGBA_GREEN_MASK,
                                                       FI_RGBA_BLUE_MASK,
                                                       isTopDown);
        if (is_set(exportFlags, ExportFlags::ExportAlpha) == false || fileFormat == Bitmap::FileFormat::JpegFile) {
            pImage = FreeImage_ConvertTo24Bits(pTemp);
            FreeImage_Unload(pTemp);
        }
        else {
            pImage = pTemp;
        }

        std::vector<std::string> warnings;
        switch (fileFormat) {
        case FileFormat::JpegFile:
            if (is_set(exportFlags, ExportFlags::Lossy) == false || is_set(exportFlags, ExportFlags::Uncompressed)) {
                flags = JPEG_QUALITYSUPERB | JPEG_SUBSAMPLING_444;
            }
            if (is_set(exportFlags, ExportFlags::ExportAlpha)) {
                Log::Warning("Bitmap::SaveImage: JPEG format does not support alpha channel.", '\n');
            }
            break;

        // Lossless formats
        case FileFormat::PngFile:
            flags = is_set(exportFlags, ExportFlags::Uncompressed) ? PNG_Z_NO_COMPRESSION : PNG_Z_BEST_COMPRESSION;

            if (is_set(exportFlags, ExportFlags::Lossy)) {
                Log::Warning("Bitmap::SaveImage: PNG format does not support lossy compression mode.", '\n');
            }
            break;

        case FileFormat::TgaFile:
            if (is_set(exportFlags, ExportFlags::Lossy)) {
                Log::Warning("Bitmap::SaveImage: TGA format does not support lossy compression mode.", '\n');
            }
            break;

        case FileFormat::BmpFile:
            if (is_set(exportFlags, ExportFlags::Lossy)) {
                Log::Warning("Bitmap::SaveImage: BMP format does not support lossy compression mode.", '\n');
            }
            if (is_set(exportFlags, ExportFlags::ExportAlpha)) {
                Log::Warning("Bitmap::SaveImage: BMP format does not support alpha channel.", '\n');
            }
            break;

        default: assert(false);
        }
    }

    if (!FreeImage_Save(toFreeImageFormat(fileFormat), pImage, path.string().c_str(), flags)) {
        Log::Error("Bitmap::SaveImage: FreeImage failed to save image", '\n');
    }
    FreeImage_Unload(pImage);
}

uint32_t Bitmap::GetLength() const
{
    return size.y * rowPitch;
}

Bitmap::FileFormat Bitmap::GetFormatFromFileExtension(const std::string& ext)
{
    static const char* Extensions[] = {".png", ".jpg", ".tga", ".bmp", ".pfm", ".exr", ".dds"};

    for (uint32_t i = 0; i < sizeof(Extensions) / sizeof(Extensions[0]); ++i) {
        if (ext == Extensions[i]) return static_cast<FileFormat>(i);
    }

    Log::Error("Unknown file format: ", ext, '\n');
    return Bitmap::FileFormat(-1);
}

std::string Bitmap::GetFileExtensionFromResouceFormat(ResourceFormat format)
{
    static const char* Extensions[] = {".png", ".jpg", ".tga", ".bmp", ".pfm", ".exr", ".dds"};

    return Extensions[static_cast<uint32_t>(format)];
}
}   // namespace MapleLeaf
