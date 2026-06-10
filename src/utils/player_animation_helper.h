#pragma once

#include "core/component.h"
#include "utils/animation_helper.h"
#include "prefab.h"
#include <spdlog/spdlog.h>

class PlayerAnimationHelper {
public:
    static constexpr int IDLE_ANIM_INDEX = 0;
    static constexpr int WALK_ANIM_INDEX = 1;

    static void UpdateAnimation(AnimatorComponent* animator, Prefab* playerPrefab, bool isMoving) {
        if (!animator || !playerPrefab || !playerPrefab->rootModel) return;

        auto& animations = playerPrefab->rootModel->animations;

        if (animations.size() <= WALK_ANIM_INDEX) {
            return;
        }

        AnimationClip* targetClip = nullptr;

        if (isMoving) {
            targetClip = &animations[WALK_ANIM_INDEX];
        }
        else {
            targetClip = &animations[IDLE_ANIM_INDEX];
        }

        AnimationHelper::Play(animator, targetClip, true, 1.0f);
    }
};