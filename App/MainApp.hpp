#pragma once

#include "App.hpp"
#include <string>

using namespace MapleLeaf;

namespace MapleLeafApp {
// Enum to track current renderer type
enum class RendererType
{
    Default,
    Deferred,
    Else
};

class MainApp : public App
{
public:
    MainApp();
    ~MainApp();

    void Start() override;
    void Update() override;

    bool Exchanged() override { return exchangedPipeline || reloadedScene; }

private:
    std::string  scenePath;
    bool         sceneLoaded      = false;
    RendererType currentRenderer  = RendererType::Default;   // Track current renderer type
    RendererType selectedRenderer = RendererType::Default;   // Default renderer type

    bool exchangedPipeline = false;
    bool reloadedScene     = false;

    void RegisterImGui();
};
}   // namespace MapleLeafApp