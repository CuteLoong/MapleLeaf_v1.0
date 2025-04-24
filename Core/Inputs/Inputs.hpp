#pragma once

#include "Devices.hpp"
#include "Graphics.hpp"
#include "KeyEnums.hpp"
#include "Scenes.hpp"

namespace MapleLeaf {

class Inputs : public Module::Registrar<Inputs>
{
    inline static const bool Registered = Register(Stage::Pre, Requires<Devices, Graphics, Scenes>());

public:
    Inputs();

    void Update() override;

    void ResetPositionDelta() { positionDelta = glm::vec3(0.0f); }
    void ResetRotationDelta() { rotationDelta = glm::vec2(0.0f); }
    void ResetScrollDelta() { scrollDelta = 0.0f; }

    const glm::vec3 GetPositionDelta() { return std::exchange(positionDelta, glm::vec3(0.0f)); }
    const glm::vec2 GetRotationDelta() { return std::exchange(rotationDelta, glm::vec2(0.0f)); }
    const float     GetScrollDelta() { return std::exchange(scrollDelta, 0.0f); }
    bool            GetSpacePressed() { return std::exchange(spacePressed, false); }

private:
    glm::vec3 positionDelta;
    glm::vec2 rotationDelta;
    float     scrollDelta;

    bool cusorLeftPress = false;
    bool spacePressed   = false;

    void ProcessMouseButton(MouseButton mouseButton, InputAction inputAction, InputMod inputMod);
    void ProcessKeyboard(Key key, InputAction inputAction, InputMod inputMod);
    void ProcessMousePosition(glm::vec2 value);
};
}   // namespace MapleLeaf