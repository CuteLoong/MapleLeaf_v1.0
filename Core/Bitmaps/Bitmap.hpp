#pragma once

#include "glm/glm.hpp"
#include <filesystem>

#include "Macros.hpp"
#include "ResourceFormat.h"


namespace MapleLeaf {
class Bitmap
{
public:
    enum class FileFormat
    {
        PngFile,    //< PNG file for lossless compressed 8-bits images with optional alpha
        JpegFile,   //< JPEG file for lossy compressed 8-bits images without alpha
        TgaFile,    //< TGA file for lossless uncompressed 8-bits images with optional alpha
        BmpFile,    //< BMP file for lossless uncompressed 8-bits images with optional alpha
        PfmFile,    //< PFM file for floating point HDR images with 32-bit float per channel
        ExrFile,    //< EXR file for floating point HDR images with 16-bit float per channel
        DdsFile,    //< DDS file for storing GPU resource formats, including block compressed formats
    };

    enum class ExportFlags : uint32_t
    {
        None         = 0u,        //< Default
        ExportAlpha  = 1u << 0,   //< Save alpha channel as well
        Lossy        = 1u << 1,   //< Try to store in a lossy format
        Uncompressed = 1u << 2,   //< Prefer faster load to a more compact file size
    };

    Bitmap() = default;

    explicit Bitmap(std::filesystem::path filename);
    Bitmap(const glm::uvec2& size, ResourceFormat format = ResourceFormat::RGBA8Unorm);
    Bitmap(std::unique_ptr<uint8_t[]>&& data, const glm::uvec2& size, ResourceFormat format = ResourceFormat::RGBA8Unorm);
    ~Bitmap() = default;

    void ConvertBGRAtoRGBA(uint8_t* bgraData, uint32_t width, uint32_t height) const;
    void GammaCorrect(float gamma = 2.2f);

    void Load(const std::filesystem::path& filename);
    void Write(const std::filesystem::path& filename) const;
    static void SaveImage(const std::filesystem::path& path, uint32_t width, uint32_t height, FileFormat fileFormat, ExportFlags exportFlags,
                   ResourceFormat resourceFormat, bool isTopDown, void* pData);

    explicit operator bool() const noexcept { return !data; }

    uint32_t GetLength() const;

    const std::filesystem::path& GetFilename() const { return filename; }
    void                         SetFilename(const std::filesystem::path& filename) { this->filename = filename; }

    const std::unique_ptr<uint8_t[]>& GetData() const { return data; }
    std::unique_ptr<uint8_t[]>&       GetData() { return data; }
    void                              SetData(std::unique_ptr<uint8_t[]>&& data) { this->data = std::move(data); }

    const glm::uvec2& GetSize() const { return size; }
    void              SetSize(const glm::uvec2& size) { this->size = size; }

    uint32_t       GetComponentCount() const { return componentCount; }
    ResourceFormat GetFormat() const { return format; }

    static FileFormat  GetFormatFromFileExtension(const std::string& ext);
    static std::string GetFileExtensionFromResouceFormat(ResourceFormat format);

private:
    std::filesystem::path      filename;
    std::unique_ptr<uint8_t[]> data;
    glm::uvec2                 size;
    uint32_t                   rowPitch;   // The number of bytes between the start of each row of pixels

    uint32_t componentCount;

    ResourceFormat format;
};
MAPLELEAF_ENUM_CLASS_OPERATORS(Bitmap::ExportFlags);
}   // namespace MapleLeaf