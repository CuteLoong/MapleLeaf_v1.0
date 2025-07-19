#include "FrameRecord.hpp"
#include "Graphics.hpp"
#include "Imgui.hpp"
#include "Scenes.hpp"

#include "config.h"

namespace MapleLeaf {
FrameRecord::FrameRecord()
    : recording(false)
    , screenshotPath(std::string(CONFIG_PROJECT_DIR) + "/build/Screenshots")
    , captureMode(CaptureMode::Image)
    , seriesExtent({0, 0})
{}
FrameRecord::~FrameRecord() {}

void FrameRecord::Update()
{
    RegisterImGui();

    if (recording) {
        const auto& camera = Scenes::Get()->GetScene()->GetCamera();
        if (captureMode == CaptureMode::Image) {
            Graphics::Get()->CaptureImage2d(screenshotPath / (attachmentName + "_" + Time::GetDateTime("%Y%m%d%H%M%S.exr")),
                                            dynamic_cast<const Image*>(Graphics::Get()->GetAttachment(attachmentName)));
            recording = false;
        }
        else if (captureMode == CaptureMode::ImageSeries) {
            if (Scenes::Get()->GetScene()->GetCamera()->GetFrameID() < seriesExtent.second) {
                Graphics::Get()->CaptureImage2d(
                    screenshotPath / (attachmentName + "_" + std::to_string(Scenes::Get()->GetScene()->GetCamera()->GetFrameID()) + ".exr"),
                    dynamic_cast<const Image*>(Graphics::Get()->GetAttachment(attachmentName)));
            }
            else {
                recording = false;
            }
        }
    }
}

void FrameRecord::RegisterImGui()
{
    if (auto* imgui = Imgui::Get()) {
        imgui->RegisterCustomWindow(typeid(*this).name(), [this]() {
            ImGui::SetNextItemWidth(200.0f);
            ImGui::Text("Captured Path: %s", screenshotPath.string().c_str());
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::Button("Set Path")) {
                std::string tmpPath;
                if (Imgui::Get()->OpenFolderDialog(tmpPath, "Select recording path")) {
                    screenshotPath = tmpPath;
                }
            }

            ImGui::SetNextItemWidth(100.0f);
            static const char* captureModes[] = {"Image", "Image Series"};
            int                currentMode    = static_cast<int>(captureMode);
            if (ImGui::Combo("Capture Mode", &currentMode, captureModes, IM_ARRAYSIZE(captureModes))) {
                captureMode = static_cast<CaptureMode>(currentMode);
            }

            ImGui::SetNextItemWidth(100.0f);
            std::vector<std::string> attachmentNames = Graphics::Get()->GetAttachmentNames();
            if (ImGui::BeginCombo("AttachmentName", attachmentName.c_str())) {
                for (const auto& name : attachmentNames) {
                    bool isSelected = (attachmentName == name);
                    if (ImGui::Selectable(name.c_str(), isSelected)) {
                        attachmentName = name;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (captureMode == CaptureMode::ImageSeries) {
                int extent[2] = {int(seriesExtent.first), int(seriesExtent.second)};
                ImGui::SetNextItemWidth(100.0f);
                if (ImGui::InputInt2("Series Extent", extent)) {
                    seriesExtent.first  = static_cast<uint32_t>(extent[0]);
                    seriesExtent.second = static_cast<uint32_t>(extent[1]);
                }
            }

            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::Button("Recording")) {
                recording = true;
                if (captureMode == CaptureMode::ImageSeries) {
                    Scenes::Get()->GetScene()->GetCamera()->SetFrameID(seriesExtent.first);
                }
            }
        });
    }
}
}   // namespace MapleLeaf