#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#define GLM_ENABLE_EXPERIMENTAL

#include "core/ecs.h"
#include "core/component.h"
#include "animation/skeleton.h"
#include "animation/animation_clip.h"
#include <glm/glm.hpp>

class AnimationSystem : public System {
private:
    Query<AnimatorComponent>* query;

    void CalculateBoneTransform(const SkeletonNode* node, const glm::mat4& parentTransform, AnimatorComponent* animator);

    glm::vec3 CalcInterpolatedPosition(float animationTime, const AnimationChannel& channel, int& lastIndex);
    glm::quat CalcInterpolatedRotation(float animationTime, const AnimationChannel& channel, int& lastIndex);
    glm::vec3 CalcInterpolatedScale(float animationTime, const AnimationChannel& channel, int& lastIndex);

    int GetPositionIndex(float animationTime, const AnimationChannel& channel, int& lastIndex);
    int GetRotationIndex(float animationTime, const AnimationChannel& channel, int& lastIndex);
    int GetScaleIndex(float animationTime, const AnimationChannel& channel, int& lastIndex);

public:
    AnimationSystem(ECS& ecs);

    void OnGameObjectUpdated(GameObject* e) override;

    void Update(ECS& ecs, float) override;
};

#endif