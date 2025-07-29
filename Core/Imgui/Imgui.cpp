#include "Imgui.hpp"
#include "Graphics.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan_core.h"
#define NOMINMAX
#include <ShlObj.h>
#include <windows.h>

namespace MapleLeaf {

Imgui::Imgui()
{
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    // ImFont* font = io.Fonts->AddFontFromFileTTF("Fonts/DroidSans.ttf", 18.0f);
    // io.FontDefault = font;
    ImGui_ImplGlfw_InitForVulkan(Devices::Get()->GetWindow()->GetWindow(), true);
}

Imgui::~Imgui()
{
    auto logicalDevice = Graphics::Get()->GetLogicalDevice();

    auto graphicsQueue = logicalDevice->GetGraphicsQueue();
    auto computeQueue  = logicalDevice->GetComputeQueue();

    if (graphicsQueue) Graphics::CheckVk(vkQueueWaitIdle(graphicsQueue));
    if (computeQueue) Graphics::CheckVk(vkQueueWaitIdle(computeQueue));

    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplVulkan_Shutdown();
}

void Imgui::SetupImGui(const VkRenderPass& renderpass)
{
    if (this->renderpass == VK_NULL_HANDLE || renderpass != this->renderpass) {
        if (this->renderpass != VK_NULL_HANDLE) ImGui_ImplVulkan_Shutdown();

        this->renderpass = renderpass;
        imageDescriptors.clear();

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.ApiVersion                = VK_API_VERSION_1_4;
        initInfo.Instance                  = *Graphics::Get()->GetInstance();
        initInfo.PhysicalDevice            = *Graphics::Get()->GetPhysicalDevice();
        initInfo.Device                    = *Graphics::Get()->GetLogicalDevice();
        initInfo.QueueFamily               = Graphics::Get()->GetLogicalDevice()->GetGraphicsFamily();
        initInfo.Queue                     = Graphics::Get()->GetLogicalDevice()->GetGraphicsQueue();
        initInfo.PipelineCache             = Graphics::Get()->GetPipelineCache();
        initInfo.DescriptorPoolSize        = 8192;
        initInfo.RenderPass                = renderpass;
        initInfo.Subpass                   = 1;
        initInfo.MinImageCount             = 3;
        initInfo.ImageCount                = 3;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator                 = nullptr;
        initInfo.CheckVkResultFn           = Graphics::CheckVk;

        ImGui_ImplVulkan_Init(&initInfo);
        ImGui::StyleColorsDark();

        Update();   // Initialize ImGui with the current state
    }
}

void Imgui::Update()
{
    if (!renderpass) return;

    // ImGuiIO& io    = ImGui::GetIO();
    // io.DisplaySize = ImVec2(Devices::Get()->GetWindow()->GetSize().x, Devices::Get()->GetWindow()->GetSize().y);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(300, 300), ImGuiCond_Always);

    ImGui::Begin(Engine::Get()->GetApp()->GetName().c_str(), nullptr);
    Engine::Get()->RegisterImGui();
    // ImGui::Text("FPS : %i", Engine::Get()->GetFps());
    for (const auto& [name, func] : customImguiWindows) {
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) func();
    }

    ImGui::End();
    // ImGui::ShowDemoWindow();

    ImGui::EndFrame();
    ImGui::Render();
}

VkDescriptorSet Imgui::GetImageDescriptor(const Image* image)
{
    auto it = imageDescriptors.find(image);
    if (it == imageDescriptors.end()) {
        VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(image->GetSampler(), image->GetView(), image->GetLayout());
        imageDescriptors.insert({image, descriptorSet});
        return descriptorSet;
    }

    return it != imageDescriptors.end() ? it->second : VK_NULL_HANDLE;
}


void Imgui::Render(const CommandBuffer& commandBuffer)
{
    if (renderpass != VK_NULL_HANDLE && ImGui::GetDrawData() != nullptr) ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Imgui::RegisterCustomWindow(const std::string& name, const std::function<void()>& func)
{
    customImguiWindows[name] = func;
}

bool Imgui::OpenFileDialog(std::string& outPath, const char* filter, const char* title)
{
    char          filename[MAX_PATH] = "";
    OPENFILENAMEA ofn                = {};
    ofn.lStructSize                  = sizeof(ofn);
    ofn.hwndOwner                    = nullptr;
    ofn.lpstrFile                    = filename;
    ofn.nMaxFile                     = MAX_PATH;
    ofn.lpstrFilter                  = filter;
    ofn.lpstrTitle                   = title;
    ofn.Flags                        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        outPath = filename;
        return true;
    }
    return false;
}

bool Imgui::OpenFolderDialog(std::string& outPath, const char* title)
{
    BROWSEINFOA bi = {0};   // Initialize to zeros
    bi.lpszTitle   = title;
    bi.ulFlags     = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;   // Key flags for folder selection

    // Call the shell function to open the folder browser dialog
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

    if (pidl != nullptr) {
        // If the user selected a folder, convert the PIDL to a file system path
        char pathBuffer[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, pathBuffer)) {
            outPath = pathBuffer;
            // Free the PIDL returned by SHBrowseForFolder
            CoTaskMemFree(pidl);
            return true;
        }
        // Free the PIDL even if SHGetPathFromIDListA fails
        CoTaskMemFree(pidl);
    }
    return false;
}

}   // namespace MapleLeaf