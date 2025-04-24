#pragma once

#include "Renderer.hpp"

using namespace MapleLeaf;

namespace MapleLeafApp {
class DefaultRenderer : public Renderer
{
public:
    DefaultRenderer();

    void Start() override;
    void Update() override;

private:
    Pipeline::Stage triangleStage;
};
}   // namespace MapleLeafApp