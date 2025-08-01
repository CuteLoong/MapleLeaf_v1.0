#pragma once

#include "Renderer.hpp"

using namespace MapleLeaf;

namespace MapleLeafApp {
class RayTracingRenderer : public Renderer
{
public:
    RayTracingRenderer();

    void Start() override;
    void Update() override;

private:
    Pipeline::Stage raytracingStage;
};
}   // namespace MapleLeafApp
