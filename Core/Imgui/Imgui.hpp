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

    bool cmdRender(const CommandBuffer& commandBuffer);

    void RegisterCustomWindow(const std::string& name, const std::function<void()>& func);

    bool OpenFileDialog(std::string& outPath, const char* filter = "All Files\0*.*\0", const char* title = "Open File");

    const glm::vec2& GetScale() const { return scale; }
    const glm::vec2& GetTranslate() const { return translate; }

    const Image2d* GetFontImage() const { return fontImage.get(); }

    bool GetImguiCursorState() const { return ImGui::GetIO().WantCaptureMouse; }

    static Shader::VertexInput GetVertexInput(uint32_t baseBinding = 0)
    {
        std::vector<VkVertexInputBindingDescription>   bindingDescriptions   = {{baseBinding, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX}};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
            {0, baseBinding, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},
            {1, baseBinding, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},
            {2, baseBinding, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)}};
        return {bindingDescriptions, attributeDescriptions};
    }

private:
    std::unique_ptr<Buffer>  vertexBuffer;
    std::unique_ptr<Buffer>  indexBuffer;
    std::unique_ptr<Image2d> fontImage;

    glm::vec2 scale, translate;   // for shader

    std::unordered_map<std::string, std::function<void()>> customImguiWindows;

    void UpdateDrawData();
    void SetImguiDrawData(ImDrawData* imDrawData);
};
}   // namespace MapleLeaf