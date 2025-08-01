#include "MainApp.hpp"
#include "DefaultRenderer.hpp"
#include "DeferredRenderer.hpp"
#include "Engine.hpp"
#include "FrameRecord.hpp"
#include "Imgui.hpp"
#include "Inputs.hpp"
#include "Log.hpp"
#include "RayTracingRenderer.hpp"
#include "SceneBuilder.hpp"
#include "Scenes.hpp"
#include "SkyboxMappingRenderer.hpp"
#include <windows.h>

#include "config.h"

int main(int argc, char** argv)
{
    auto engine = std::make_unique<Engine>(argv[0]);
    engine->SetApp(std::make_unique<MapleLeafApp::MainApp>());

    auto exitCode = engine->Run();
    engine        = nullptr;

    std::cout << "Press enter to continue...";
    std::cin.get();
    return exitCode;
}

namespace MapleLeafApp {
MainApp::MainApp()
    : App("MapleLeaf", {CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR, CONFIG_VERSION_ALTER})
{
    // Registers file search paths.
    Log::Out("Working Directory: ", std::filesystem::current_path(), '\n');
    Files::Get()->AddSearchPath("Resources");
    Files::Get()->AddSearchPath("Resources/Shader");
    Files::Get()->AddSearchPath("Resources/Skybox");
}

MainApp::~MainApp() {}

void MainApp::Start()
{
    Devices::Get()->GetWindow()->SetTitle("MapleLeaf");
    Graphics::Get()->SetRenderer(std::make_unique<DefaultRenderer>());

#ifdef MAPLELEAF_RAY_TRACING
    // Ray tracing is not supported in this simplified version
    currentRenderer = RendererType::Default;   // Update tracker to match
#else
    // Use our default renderer
    currentRenderer = RendererType::Default;   // Update tracker to match
#endif

    sceneLoaded = false;
    scenePath.clear();
}

void MainApp::RegisterImGui()
{
    Imgui::Get()->RegisterCustomWindow(typeid(*this).name(), [this]() {
        if (!sceneLoaded) {
            ImGui::Text("Scene loaded:");

            if (ImGui::Button("Load Scene")) {
                if (Imgui::Get()->OpenFileDialog(scenePath, "GLTF Files\0*.gltf\0", "Load Scene")) sceneLoaded = false;
            }
        }
        else {
            ImGui::Text("Scene loaded: %s", scenePath.c_str());

            if (ImGui::Button("ReLoad Scene")) {
                if (Imgui::Get()->OpenFileDialog(scenePath, "GLTF Files\0*.gltf\0", "Load Scene")) sceneLoaded = false;
            }
        }

        ImGui::SetNextItemWidth(150.0f);
        static const char* rendererNames[] = {"Default", "Deferred", "Ray Tracing"};
        int                currentItem     = static_cast<int>(selectedRenderer);
        if (ImGui::Combo("Renderer", &currentItem, rendererNames, IM_ARRAYSIZE(rendererNames))) {
            switch (currentItem) {
            case 0: selectedRenderer = RendererType::Default; break;
            case 1: selectedRenderer = RendererType::Deferred; break;
            case 2: selectedRenderer = RendererType::RayTracing; break;
            default: Log::Error("Unknown renderer type selected: ", currentItem); return;
            }
        }
    });
}

void MainApp::Update()
{
    if (Devices::Get()->GetWindow()->IsClosed()) {
        Engine::Get()->RequestClose();
    }

    exchangedPipeline  = false;
    bool reloadedScene = false;

    RegisterImGui();

    if (!sceneLoaded) {
        std::filesystem::path userPath(scenePath);
        if (!userPath.empty()) {
            auto scene = std::make_unique<SceneBuilder>(userPath);
            Scenes::Get()->SetScene(std::move(scene));
            sceneLoaded   = true;
            reloadedScene = true;

            Imgui::Get()->ClearCustomWindows();
        }
    }

    if (const auto* scene = Scenes::Get()->GetScene()) {
        if (const auto* skyboxSystem = scene->GetSystem<SkyboxSystem>(); skyboxSystem->WaitMapping()) {
            Graphics::Get()->SetRenderer(std::make_unique<SkyboxMappingRenderer>());
            exchangedPipeline = true;
            currentRenderer   = RendererType::Else;
            Imgui::Get()->ClearCustomWindows();
        }
        else if (sceneLoaded) {
            if (selectedRenderer != currentRenderer || Inputs::Get()->GetF5Pressed()) {
                if (selectedRenderer == RendererType::Default) {
                    Graphics::Get()->SetRenderer(std::make_unique<DefaultRenderer>());
                    currentRenderer   = RendererType::Default;
                    exchangedPipeline = true;
                    Imgui::Get()->ClearCustomWindows();
                }
                else if (selectedRenderer == RendererType::Deferred) {
                    Graphics::Get()->SetRenderer(std::make_unique<DeferredRenderer>());
                    currentRenderer   = RendererType::Deferred;
                    exchangedPipeline = true;
                    Imgui::Get()->ClearCustomWindows();
                }
                else if (selectedRenderer == RendererType::RayTracing) {
                    Graphics::Get()->SetRenderer(std::make_unique<RayTracingRenderer>());
                    currentRenderer   = RendererType::RayTracing;
                    exchangedPipeline = true;
                    Imgui::Get()->ClearCustomWindows();
                }
                else {
                    Log::Error("Unknown renderer type selected: ", static_cast<int>(selectedRenderer));
                }
            }
        }
    }
}
}   // namespace MapleLeafApp