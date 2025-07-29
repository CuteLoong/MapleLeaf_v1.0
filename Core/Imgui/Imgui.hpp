#pragma once

#include "Buffer.hpp"
#include "Devices.hpp"
#include "Graphics.hpp"
#include "Image2d.hpp"
#include "Imgui.h"
#include "Module.hpp"
#include "Shader.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace MapleLeaf {
class Imgui : public Module::Registrar<Imgui>
{
    inline static const bool Registered = Register(Stage::Always, Requires<Devices, Graphics>());

public:
    Imgui();
    ~Imgui();

    void Update() override;
    void SetupImGui(const VkRenderPass& renderpass);

    VkDescriptorSet GetImageDescriptor(const Image* image);

    void Render(const CommandBuffer& commandBuffer);

    void RegisterCustomWindow(const std::string& name, const std::function<void()>& func);
    void ClearCustomWindows() { customImguiWindows.clear(); }

    bool OpenFileDialog(std::string& outPath, const char* filter = "All Files\0*.*\0", const char* title = "Open File");
    bool OpenFolderDialog(std::string& outPath, const char* title);

    bool GetImguiCursorState() const { return ImGui::GetIO().WantCaptureMouse; }

private:
    VkRenderPass renderpass = VK_NULL_HANDLE;

    std::unordered_map<const Image*, VkDescriptorSet> imageDescriptors;

    std::unordered_map<std::string, std::function<void()>> customImguiWindows;
};
}   // namespace MapleLeaf