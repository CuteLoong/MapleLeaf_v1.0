#include "Transform.hpp"
#include "AnimationController.hpp"
#include "Entity.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace MapleLeaf {
Transform::Transform(const glm::vec3& position, const glm::quat& quaternion, const glm::vec3& scale)
    : position(position)
    , quaternion(quaternion)
    , scale(scale)
    , prevPosition(position)
    , prevQuaterion(quaternion)
    , prevScale(scale)
{
    updateStatus = UpdateStatus::Transformation;
}

Transform::Transform(const glm::mat4 modelMatrix)
{
    glm::vec3 scale;
    glm::quat quaternion;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(modelMatrix, scale, quaternion, translation, skew, perspective);

    this->position   = translation;
    this->quaternion = quaternion;
    this->scale      = scale;

    this->prevPosition  = position;
    this->prevQuaterion = quaternion;
    this->prevScale     = scale;

    updateStatus = UpdateStatus::Transformation;
}

Transform::Transform(const glm::vec3 position, const glm::quat quaternion, const glm::vec3 scale, const glm::vec3 prevPosition,
                     const glm::quat prevQuaternion, const glm::vec3 prevScale)
    : position(position)
    , quaternion(quaternion)
    , scale(scale)
    , prevPosition(prevPosition)
    , prevQuaterion(quaternion)
    , prevScale(prevScale)
{}

Transform::~Transform()
{
    if (worldTransform) delete worldTransform;

    for (auto& child : children) child->parent = nullptr;

    if (parent) parent->RemoveChild(this);
}

void Transform::Update()
{
    prevPosition  = position;
    prevQuaterion = quaternion;
    prevScale     = scale;

    if (const auto& ani = this->GetEntity()->GetComponent<AnimationController>(); ani != nullptr && ani->isMatrixChanged()) {
        glm::mat4 modelMatrix = ani->getLocalMatrix();

        glm::vec3 scale;
        glm::quat quaternion;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(modelMatrix, scale, quaternion, translation, skew, perspective);

        position = translation;

        this->position   = translation;
        this->quaternion = quaternion;
        this->scale      = scale;

        updateStatus = UpdateStatus::Transformation;
    }
    else {
        if (this->parent != nullptr && this->parent->GetUpdateStatus() == UpdateStatus::Transformation)
            updateStatus = UpdateStatus::Transformation;
        else
            updateStatus = UpdateStatus::None;
    }
}

glm::mat4 Transform::GetWorldMatrix() const
{
    auto worldTransform = GetWorldTransform();

    glm::mat4 scaleMatrix       = glm::scale(glm::mat4(1.0f), worldTransform->scale);
    glm::mat4 rotationMatrix    = glm::mat4_cast(worldTransform->quaternion);
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), worldTransform->position);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

glm::mat4 Transform::GetPrevWorldMatrix() const
{
    auto worldTransform = GetWorldTransform();

    glm::mat4 scaleMatrix       = glm::scale(glm::mat4(1.0f), worldTransform->prevScale);
    glm::mat4 rotationMatrix    = glm::mat4_cast(worldTransform->prevQuaterion);
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), worldTransform->prevPosition);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

glm::vec3 Transform::GetPosition() const
{
    return GetWorldTransform()->position;
}

glm::vec3 Transform::GetRotation() const
{
    return glm::eulerAngles(quaternion);
}

glm::vec3 Transform::GetScale() const
{
    return GetWorldTransform()->scale;
}

glm::vec3 Transform::GetPrevPosition() const
{
    return GetWorldTransform()->prevPosition;
}

glm::vec3 Transform::GetPrevRotation() const
{
    return glm::eulerAngles(prevQuaterion);
}

glm::vec3 Transform::GetPrevScale() const
{
    return GetWorldTransform()->prevScale;
}

void Transform::SetParent(Transform* parent)
{
    if (this->parent) this->parent->RemoveChild(this);

    this->parent = parent;

    if (this->parent) this->parent->AddChild(this);
}

void Transform::SetParent(Entity* parent)
{
    SetParent(parent->GetComponent<Transform>());
}

bool Transform::operator==(const Transform& rhs) const
{
    return position == rhs.position && quaternion == rhs.quaternion && scale == rhs.scale;
}

bool Transform::operator!=(const Transform& rhs) const
{
    return !operator==(rhs);
}

Transform operator*(const Transform& lhs, const Transform& rhs)
{
    glm::vec4 newPosition = glm::vec4(rhs.position, 1.0f);
    // for (uint32_t row = 0; row < 4; row++) {
    //     newPosition[row] = lhs.GetWorldMatrix()[row][0] * rhs.position.x + lhs.GetWorldMatrix()[row][1] * rhs.position.y +
    //                        lhs.GetWorldMatrix()[row][2] * rhs.position.z + lhs.GetWorldMatrix()[row][3] * 1.0f;
    // }
    newPosition = lhs.GetWorldMatrix() * newPosition;

    glm::vec4 prevNewPosition = glm::vec4(rhs.prevPosition, 1.0f);
    prevNewPosition           = lhs.GetPrevWorldMatrix() * prevNewPosition;

    return {glm::vec3(newPosition),
            lhs.quaternion * rhs.quaternion,
            lhs.scale * rhs.scale,
            glm::vec3(prevNewPosition),
            lhs.prevQuaterion * rhs.prevQuaterion,
            lhs.prevScale * rhs.prevScale};
}

Transform& Transform::operator*=(const Transform& rhs)
{
    return *this = *this * rhs;
}

std::ostream& operator<<(std::ostream& stream, const Transform& transform)
{
    return stream << transform.position << ", " << transform.GetRotation() << ", " << transform.scale << ", " << transform.prevPosition << ", "
                  << transform.GetPrevRotation() << ", " << transform.prevScale;
}

const Transform* Transform::GetWorldTransform() const
{
    if (!parent) {
        if (worldTransform) {
            delete worldTransform;
            worldTransform = nullptr;
        }

        return this;
    }

    if (!worldTransform) {
        worldTransform = new Transform();
    }

    *worldTransform = *parent->GetWorldTransform() * *this;
    return worldTransform;
}

void Transform::AddChild(Transform* child)
{
    children.emplace_back(child);
}

void Transform::RemoveChild(Transform* child)
{
    children.erase(std::remove(children.begin(), children.end(), child), children.end());
}
}   // namespace MapleLeaf