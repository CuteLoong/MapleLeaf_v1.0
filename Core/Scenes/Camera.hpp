#pragma once

#include "Component.hpp"
#include "UniformHandler.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <array>
#include <string>

namespace MapleLeaf {
class Camera : public Component::Registrar<Camera>
{
public:
    Camera();

    ~Camera() = default;

    void Start();
    void Update();

    void UpdateByTransform();
    void UpdateByInput();

    void UpdateCameraInfo();
    void UpdateStereoCameraInfo();

    void PushUniforms(UniformHandler& uniformObject);

    /**
     * Gets the distance of the near pane of the view frustum.
     * @return The distance of the near pane of the view frustum.
     */
    float GetNearPlane() const { return nearPlane; }
    void  SetNearPlane(float nearPlane) { this->nearPlane = nearPlane; }

    /**
     * Gets the distance of the view frustum's far plane.
     * @return The distance of the view frustum's far plane.
     */
    float GetFarPlane() const { return farPlane; }
    void  SetFarPlane(float farPlane) { this->farPlane = farPlane; }

    /**
     * Gets the field of view angle for the view frustum.
     * @return The field of view angle for the view frustum.
     */
    float GetFieldOfView() const { return fieldOfView; }
    void  SetFieldOfView(float fieldOfView) { this->fieldOfView = fieldOfView; }

    float GetAspectRatio() const { return aspectRatio; }
    void  SetAspectRatio(float aspectRatio) { this->aspectRatio = aspectRatio; }

    const glm::vec3& GetPosition() const { return position; }
    const glm::vec3& GetRotation() const { return rotation; }
    const glm::vec3& GetVelocity() const { return velocity; }
    const glm::vec3& GetUpVector() const { return up; }
    const glm::vec3& GetForward() const { return forward; }
    const glm::vec3& GetRight() const { return right; }
    void             SetUpVector(const glm::vec3& up) { this->up = up; }
    void             SetForward(const glm::vec3& forward) { this->forward = forward; }
    void             SetPosition(const glm::vec3& position) { this->position = position; }
    void             SetRotation(const glm::vec3& rotation) { this->rotation = rotation; }
    void             SetVelocity(const glm::vec3& velocity) { this->velocity = velocity; }

    const std::string& GetName() const { return name; }
    void               SetName(const std::string& name) { this->name = name; }

    const glm::mat4& GetViewMatrix() const { return viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }

    const glm::mat4& GetInverseViewMatrix() const { return invViewMatrix; }
    const glm::mat4& GetInverseProjectionMatrix() const { return invProjectionMatrix; }

    // Values used to linearize the Z buffer (http://www.humus.name/temp/Linearize%20depth.txt)
    // x = 1-far/near
    // y = far/near
    // z = x/far
    // w = y/far
    const glm::vec4 GetZBufferParams() const;
    const glm::vec4 GetProjectionParams() const;

    const uint32_t  GetPixelHeight() const;
    const uint32_t  GetPixelWidth() const;
    const glm::vec4 GetPixelSize() const;

    const uint32_t  GetStereoPixelHeight() const;
    const uint32_t  GetStereoPixelWidth() const;
    const glm::vec4 GetStereoPixelSize() const;

    const std::array<glm::vec4, 2>& GetStereoViewPosition() const { return stereoViewPosition; }

    const std::array<glm::mat4, 2>& GetStereoViewMatrix() const { return stereoViewMatrix; }
    const std::array<glm::mat4, 2>& GetStereoProjectionMatrix() const { return stereoProjectionMatrix; }
    const std::array<glm::mat4, 2>& GetInvStereoViewMatrix() const { return invStereoViewMatrix; };
    const std::array<glm::mat4, 2>& GetInvStereoProjectionMatrix() const { return invStereoProjectionMatrix; }

    uint32_t frameID;   // this camera's frame id

protected:
    std::string name;

    float nearPlane, farPlane;
    float fieldOfView, aspectRatio;
    float eyeSeparation;
    float orthoScale;

    glm::vec3 position;
    glm::vec3 rotation;   // (0, 0, -1.0) ROTATION
    glm::vec3 velocity;

    glm::vec3 up;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 initialCenter;
    glm::vec3 initialRight;

    glm::mat4 viewMatrix;
    glm::mat4 prevViewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 prevProjectionMatrix;
    glm::mat4 orthoMatrix;
    glm::mat4 prevOrthoMatrix;
    glm::mat4 invViewMatrix;
    glm::mat4 invProjectionMatrix;

    /* frustum's four vector, start from camera position
     *0    1
     *2    3
     */
    std::array<glm::vec4, 4> frustumVector;

    /* frustum's six plane, store normal
     * left, right, bottom, top, near, far
     */
    std::array<glm::vec4, 6> frustumPlane;

    std::array<glm::vec4, 2> stereoViewPosition;

    std::array<glm::mat4, 2> stereoViewMatrix;
    std::array<glm::mat4, 2> prevStereoViewMatrix;
    std::array<glm::mat4, 2> stereoProjectionMatrix;
    std::array<glm::mat4, 2> prevStereoProjectionMatrix;
    std::array<glm::mat4, 2> invStereoViewMatrix;
    std::array<glm::mat4, 2> invStereoProjectionMatrix;

    std::array<std::array<glm::vec4, 4>, 2> stereoFrustumVector;
    std::array<std::array<glm::vec4, 6>, 2> stereoFrustumPlane;
};
}   // namespace MapleLeaf