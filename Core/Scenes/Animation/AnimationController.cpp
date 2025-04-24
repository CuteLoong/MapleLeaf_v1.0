#include "AnimationController.hpp"
#include "Animation.hpp"
#include "Scenes.hpp"

namespace MapleLeaf {
AnimationController::AnimationController(const std::shared_ptr<Animation> animation)
    : animation(animation)
{
    matrixChanged = false;
    localMatrix   = glm::mat4(1.0f);

    mGlobalAnimationLength = std::max(mGlobalAnimationLength, animation->getDuration());
}

void AnimationController::Update()
{
    Camera* camera = Scenes::Get()->GetScene()->GetCamera();

    uint32_t animationIndex = camera->frameID;
    matrixChanged           = false;

    double currentTime = (1.0 / 120.0) * animationIndex;

    double time = mLoopAnimations ? fmod(currentTime, mGlobalAnimationLength) : currentTime;

    if (mFirstUpdate || mEnabled != mPrevEnabled) {
        if (mEnabled) {
            localMatrix   = animation->animate(time);
            matrixChanged = true;
            mTime = mPrevTime = time;
        }

        mFirstUpdate = false;
        mPrevEnabled = mEnabled;
    }

    if (mEnabled && (time != mTime || mTime != mPrevTime)) {
        if (hasAnimations()) {
            localMatrix   = animation->animate(time);
            matrixChanged = true;
            mPrevTime     = time;
        }
        mPrevTime = mTime;
        mTime     = time;
    }
}

}   // namespace MapleLeaf