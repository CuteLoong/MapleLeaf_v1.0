#pragma once

#include "Module.hpp"
#include <filesystem>

namespace MapleLeaf {
enum class CaptureMode
{
    Image,
    ImageSeries
};

class FrameRecord : public Module::Registrar<FrameRecord>
{
    inline static const bool Registered = Register(Stage::Pre);

public:
    FrameRecord();
    ~FrameRecord();

    void Update() override;

private:
    std::string           attachmentName;
    std::filesystem::path screenshotPath;

    bool                          recording;
    CaptureMode                   captureMode;
    std::pair<uint32_t, uint32_t> seriesExtent;

    void RegisterImGui();
};
}   // namespace MapleLeaf