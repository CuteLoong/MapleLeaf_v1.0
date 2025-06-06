#pragma once

#include "Color.hpp"
#include "Component.hpp"
#include <array>

namespace MapleLeaf {
enum class LightType : uint8_t
{
    Directional,
    Point,
    Spot,
    Area
};

class Light : public Component::Registrar<Light>
{
    inline static const bool Registered = Register("light");

public:
    explicit Light(LightType type = LightType::Directional);

    void Start() override;
    void Update() override;

    const std::string& GetName() const { return name; }
    void               SetName(const std::string& name) { this->name = name; }

    const Color& GetColor() const { return color; }
    void         SetColor(const Color& color) { this->color = color; }

    const glm::vec3& GetPosition() const { return position; }
    void             SetPosition(const glm::vec3& position) { this->position = position; }

    const glm::vec3& GetDirection() const { return direction; }
    void             SetDirection(const glm::vec3& direction) { this->direction = direction; }

    const glm::vec3& GetAttenuation() const { return attenuationParameter; }
    void             SetAttenuation(const glm::vec3& attenuation) { this->attenuationParameter = attenuation; }

    // AreaLight
    const std::array<glm::vec3, 4>& GetPoints() const { return points; }
    void                            SetPoints(const std::array<glm::vec3, 4>& points) { this->points = points; }

    bool  GetTwoSide() const { return twoSide; }
    void  SetTwoSide(bool isTwoSide) { twoSide = isTwoSide; }
    void  SetIntensity(const float intensity) { this->intensity = intensity; }
    float GetIntensity() const { return intensity; }

    LightType type;

private:
    std::string name;
    Color       color;

    /**
     * @brief Position of the light source in space.
     * The position is undefined for directional light
     */
    glm::vec3 position;
    /**
     * @brief Direction of the light source in space.
     * The direction is undefined for point lights.
     */
    glm::vec3 direction;

    /**
     * @brief light attenuation parameters
     * attenuationParameter.x: attenuationConstant
     * attenuationParameter.y: attenuationLinear
     * attenuationParameter.z: attenuationQuadratic
     */
    glm::vec3 attenuationParameter;

    // AreaLight
    std::array<glm::vec3, 4> points;
    bool                     twoSide;
    float                    intensity;
};
}   // namespace MapleLeaf