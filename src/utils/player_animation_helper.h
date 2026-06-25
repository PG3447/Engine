#pragma once

#include "core/component.h"
#include "utils/animation_helper.h"
#include "prefab.h"

class PlayerAnimationHelper {
public:
    static constexpr int IDLE_ANIM_INDEX = 0;
    static constexpr int IDLE_HOLD_INDEX = 1;
    static constexpr int INTERACT_ANIM_INDEX = 2;
    static constexpr int PICKUP_ANIM_INDEX = 3;
    static constexpr int DROP_ANIM_INDEX = 4;
    static constexpr int TURN_AROUND_ANIM_INDEX = 5;
    static constexpr int TURN_AROUND_HOLD_INDEX = 6;
    static constexpr int WALK_ANIM_INDEX = 7;
    static constexpr int WALK_HOLD_INDEX = 8;

    static void TriggerAction(AnimatorComponent* animator, Prefab* playerPrefab, int actionIndex) {
        if (!animator || !playerPrefab || !playerPrefab->rootModel) return;
        auto& animations = playerPrefab->rootModel->animations;

        if (animations.size() <= actionIndex) {
            return;
        }

        AnimationClip* targetClip = &animations[actionIndex];

        AnimationHelper::Play(animator, targetClip, false, 1.0f);
    }

    static void UpdateAnimation(AnimatorComponent* animator, Prefab* playerPrefab, bool isMoving, bool isHoldingObject, bool isTurning = false) {
        if (!animator || !playerPrefab || !playerPrefab->rootModel) return;
        auto& animations = playerPrefab->rootModel->animations;

        if (animations.size() <= WALK_ANIM_INDEX) return;

        if (animator->currentAnimation != nullptr && !animator->looping && !animator->isFinished) {
            return;
        }

        AnimationClip* targetClip = nullptr;

        if (isMoving) {
            if (isHoldingObject && animations.size() > WALK_HOLD_INDEX) {
                targetClip = &animations[WALK_HOLD_INDEX];
            }
            else {
                targetClip = &animations[WALK_ANIM_INDEX];
            }
        }
        else if (isTurning) {
            if (isHoldingObject && animations.size() > TURN_AROUND_HOLD_INDEX) {
                targetClip = &animations[TURN_AROUND_HOLD_INDEX];
            }
            else if (animations.size() > TURN_AROUND_ANIM_INDEX) {
                targetClip = &animations[TURN_AROUND_ANIM_INDEX];
            }
            else {
                targetClip = &animations[IDLE_ANIM_INDEX];
            }
        }
        else {
            if (isHoldingObject && animations.size() > IDLE_HOLD_INDEX) {
                targetClip = &animations[IDLE_HOLD_INDEX];
            }
            else {
                targetClip = &animations[IDLE_ANIM_INDEX];
            }
        }

        if (targetClip != nullptr) {
            AnimationHelper::Play(animator, targetClip, true, 1.0f);
        }
    }
};