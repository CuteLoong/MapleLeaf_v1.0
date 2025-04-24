#include "MainApp.hpp"
#include "DefaultRenderer.hpp"
#include "DeferredRenderer.hpp"
#include "Engine.hpp"
#include "Imgui.hpp"
#include "Log.hpp"
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

#ifdef MAPLELEAF_RAY_TRACING
// Ray tracing is not supported in this simplified version
#    error "Ray tracing renderers have been removed in this simplified version"
#else
    // Use our default renderer
    Graphics::Get()->SetRenderer(std::make_unique<DefaultRenderer>());
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
    });
}

void MainApp::Update()
{
    if (Devices::Get()->GetWindow()->IsClosed()) {
        Engine::Get()->RequestClose();
    }

    if (const auto* scene = Scenes::Get()->GetScene()) {
        if (const auto* skyboxSystem = scene->GetSystem<SkyboxSystem>(); skyboxSystem->WaitMapping()) {
            // Only change to SkyboxMappingRenderer if not already using it
            if (currentRenderer != RendererType::SkyboxMapping) {
                Graphics::Get()->SetRenderer(std::make_unique<SkyboxMappingRenderer>());
                currentRenderer = RendererType::SkyboxMapping;
            }
        }
        else if (!sceneLoaded) {
            if (currentRenderer != RendererType::Default) {
                Graphics::Get()->SetRenderer(std::make_unique<DefaultRenderer>());
                currentRenderer = RendererType::Default;
            }
        }
        else if (sceneLoaded && scene->IsStarted()) {
            if (currentRenderer != RendererType::Deferred) {
                Graphics::Get()->SetRenderer(std::make_unique<DeferredRenderer>());
                currentRenderer = RendererType::Deferred;
            }
        }
    }

    RegisterImGui();

    if (!sceneLoaded) {
        std::filesystem::path userPath(scenePath);
        if (!userPath.empty()) {
            auto scene = std::make_unique<SceneBuilder>(userPath);
            Scenes::Get()->SetScene(std::move(scene));
            sceneLoaded = true;
        }
    }
}
}   // namespace MapleLeafApp