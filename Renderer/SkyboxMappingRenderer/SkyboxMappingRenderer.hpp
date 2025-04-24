#pragma once

#include "Renderer.hpp"

using namespace MapleLeaf;

namespace MapleLeafApp {
class SkyboxMappingRenderer : public Renderer
{
public:
    SkyboxMappingRenderer();

    void Start() override;
    void Update() override;

private:
    Pipeline::Stage skyboxMappingStage;
};
}   // namespace MapleLeafApp