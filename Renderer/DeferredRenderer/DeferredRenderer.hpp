#pragma once

#include "Renderer.hpp"

using namespace MapleLeaf;

namespace MapleLeafApp {
class DeferredRenderer : public Renderer
{
public:
    DeferredRenderer();

    void Start() override;
    void Update() override;

private:
    Pipeline::Stage deferredStage;
};
}   // namespace MapleLeafApp
