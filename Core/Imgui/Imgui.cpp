#include "Imgui.hpp"
#include "imgui_impl_glfw.h"
#define NOMINMAX
#include <windows.h>

namespace MapleLeaf {

Imgui::Imgui()
{
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(Devices::Get()->GetWindow()->GetWindow(), true);

    ImGuiIO& io = ImGui::GetIO();
    // ImFont* font = io.Fonts->AddFontFromFileTTF("Fonts/DroidSans.ttf", 18.0f);
    // io.FontDefault = font;

    io.FontGlobalScale         = 1.0f;
    io.DisplaySize             = ImVec2(Devices::Get()->GetWindow()->GetSize().x, Devices::Get()->GetWindow()->GetSize().y);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGuiStyle& style          = ImGui::GetStyle();
    style.ScaleAllSizes(1.0f);

    style.Colors[ImGuiCol_TitleBg]       = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_CheckMark]     = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    if (!fontImage && Graphics::Get()) {
        unsigned char* fontData;
        glm::ivec2     texSize;
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texSize.x, &texSize.y);
        std::unique_ptr<uint8_t[]> dataPtr(reinterpret_cast<uint8_t*>(fontData));

        std::unique_ptr<Bitmap> fontBitmap = std::make_unique<Bitmap>(std::move(dataPtr), texSize);
        fontImage                          = std::make_unique<Image2d>(std::move(fontBitmap), VK_FORMAT_R8G8B8A8_UNORM);
    }
}

Imgui::~Imgui()
{
    ImGui_ImplGlfw_Shutdown();
    // if only one createContext don't need destroy context
}

void Imgui::UpdateDrawData()
{
    ImDrawData* imDrawData = ImGui::GetDrawData();

    if (imDrawData == nullptr) return;

    VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) return;

    if (!vertexBuffer || vertexBuffer->GetBuffer() == VK_NULL_HANDLE || vertexBuffer->GetSize() < vertexBufferSize) {
        VkDeviceSize alignedSize = (vertexBufferSize + 63) & ~63;   // VkPhysicalDeviceLimits::nonCoherentAtomSize (64)
        vertexBuffer =
            std::make_unique<Buffer>(alignedSize,
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    if (!indexBuffer || indexBuffer->GetBuffer() == VK_NULL_HANDLE || indexBuffer->GetSize() < indexBufferSize) {
        VkDeviceSize alignedSize = (indexBufferSize + 63) & ~63;   // VkPhysicalDeviceLimits::nonCoherentAtomSize (64)

        indexBuffer = std::make_unique<Buffer>(alignedSize,
                                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    ImDrawVert* drawVertices = nullptr;
    ImDrawIdx*  drawIndices  = nullptr;

    vertexBuffer->MapMemory(reinterpret_cast<void**>(&drawVertices));
    indexBuffer->MapMemory(reinterpret_cast<void**>(&drawIndices));

    for (uint32_t i = 0; i < imDrawData->CmdListsCount; i++) {
        const ImDrawList* cmdList = imDrawData->CmdLists[i];
        memcpy(drawVertices, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(drawIndices, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        drawVertices += cmdList->VtxBuffer.Size;
        drawIndices += cmdList->IdxBuffer.Size;
    }
    vertexBuffer->FlushMappedMemory();
    indexBuffer->FlushMappedMemory();
    vertexBuffer->UnmapMemory();
    indexBuffer->UnmapMemory();

    // if (!vertexBuffer || !indexBuffer || vertexBuffer->GetBuffer() == VK_NULL_HANDLE || vertexBuffer->GetSize() != vertexBufferSize ||
    //     indexBuffer->GetBuffer() == VK_NULL_HANDLE || indexBuffer->GetSize() != indexBufferSize) {
    //     vertexBuffer = nullptr;
    //     indexBuffer  = nullptr;
    //     SetImguiDrawData(imDrawData);
    // }
}

void Imgui::Update()
{
    ImGuiIO& io    = ImGui::GetIO();
    io.DisplaySize = ImVec2(Devices::Get()->GetWindow()->GetSize().x, Devices::Get()->GetWindow()->GetSize().y);

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(300, 300), ImGuiCond_Always);

    ImGui::Begin(Engine::Get()->GetApp()->GetName().c_str(), nullptr);
    ImGui::Text("FPS : %i", Engine::Get()->GetFps());
    for (const auto& [name, func] : customImguiWindows) {
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) func();
    }

    ImGui::End();

    ImGui::Render();

    UpdateDrawData();
}

bool Imgui::cmdRender(const CommandBuffer& commandBuffer)
{
    ImGuiIO& io = ImGui::GetIO();
    scale       = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    translate   = glm::vec2(-1.0f);

    ImDrawData* imDrawData   = ImGui::GetDrawData();
    int32_t     vertexOffset = 0;
    int32_t     indexOffset  = 0;

    VkViewport viewport = {};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = static_cast<float>(io.DisplaySize.x);
    viewport.height     = static_cast<float>(io.DisplaySize.y);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    if (vertexBuffer && indexBuffer) {
        VkBuffer     vertexBuffers[1] = {vertexBuffer->GetBuffer()};
        VkDeviceSize offsets[1]       = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

        for (uint32_t i = 0; i < imDrawData->CmdListsCount; i++) {
            const ImDrawList* cmdList = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmdList->CmdBuffer.Size; j++) {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[j];
                VkRect2D         scissorRect;
                scissorRect.offset.x      = std::max((int32_t)(pcmd->ClipRect.x), 0);
                scissorRect.offset.y      = std::max((int32_t)(pcmd->ClipRect.y), 0);
                scissorRect.extent.width  = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmdList->VtxBuffer.Size;
        }
    }

    return true;
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

}   // namespace MapleLeaf