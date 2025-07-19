#include "Scenes.hpp"
#include "Imgui.hpp"

namespace MapleLeaf {
Scenes::Scenes() {}

void Scenes::SetScene(std::unique_ptr<Scene>&& scene)
{
    this->scene = std::move(scene);
    this->scene->Start();
    this->scene->started = true;
}

void Scenes::RegisterImGui()
{
    if (auto* imgui = Imgui::Get()) {
        imgui->RegisterCustomWindow("Scene", [this]() {
            if (!scene) return;
            auto* camera = scene->GetCamera();
            if (!camera) return;

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));   // Orange text
            // Optionally, increase frame padding slightly to give it more visual weight
            // ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, ImGui::GetStyle().FramePadding.y + 2.0f));

            if (ImGui::CollapsingHeader("Animation Settings")) {
                ImGui::Text("Animation Length: %.3f", scene->globalAnimationLength);
                int frameID = static_cast<int>(camera->frameID);
                if (ImGui::SliderInt("Frame ID", &frameID, 0, static_cast<int>(scene->globalAnimationLength * scene->animationFPS))) {
                    camera->frameID = static_cast<uint32_t>(frameID);
                }
                static const char* typeNames[] = {"24 FPS", "60 FPS", "120 FPS", "144 FPS", "160FPS"};
                int                currentItem = scene->animationFPS == 24    ? 0
                                                 : scene->animationFPS == 60  ? 1
                                                 : scene->animationFPS == 120 ? 2
                                                 : scene->animationFPS == 144 ? 3
                                                 : scene->animationFPS == 160 ? 4
                                                                              : -1;
                if (ImGui::Combo("Frame Rate", &currentItem, typeNames, IM_ARRAYSIZE(typeNames))) {
                    switch (currentItem) {
                    case 0: scene->animationFPS = 24; break;
                    case 1: scene->animationFPS = 60; break;
                    case 2: scene->animationFPS = 120; break;
                    case 3: scene->animationFPS = 144; break;
                    case 4: scene->animationFPS = 160; break;
                    default: scene->animationFPS = 120;
                    }
                }
            }
            if (ImGui::CollapsingHeader("Camera Properties")) {
                ImGui::InputFloat3("Position", &camera->position[0], "%.3f");
                ImGui::InputFloat3("Rotation", &camera->rotation[0], "%.3f");
            }

            ImGui::PopStyleColor(4);
            // ImGui::PopStyleVar(1);
        });
    }
}

void Scenes::Update()
{
    if (!scene) return;

    scene->Update();

    RegisterImGui();
}
}   // namespace MapleLeaf