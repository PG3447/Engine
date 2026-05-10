#ifndef ANIMATION_HELPER_H
#define ANIMATION_HELPER_H

#include "core/component.h"
#include "animation/animation_clip.h"
#include <functional>

class AnimationHelper {
public:
    static void Play(AnimatorComponent* animator, AnimationClip* clip, bool loop = true, float speed = 1.0f) {
        if (!animator || !clip) return;
        if (animator->currentAnimation == clip && !animator->isFinished) return;

        animator->currentAnimation = clip;
        animator->currentTime = 0.0f;
        animator->looping = loop;
        animator->playbackSpeed = speed;
        animator->isFinished = false;

        BindAnimation(animator);
    }

    static AnimationClip* FindAnimation(const std::vector<AnimationClip>& animations, const std::string& name) {
        for (const auto& anim : animations) {
            if (anim.name == name) return const_cast<AnimationClip*>(&anim);
        }
        return nullptr;
    }

    static void BindAnimation(AnimatorComponent* animator) {
        if (!animator || !animator->currentSkeleton || !animator->currentAnimation) return;

        animator->animCache.clear();
        animator->animCache.resize(animator->currentSkeleton->totalNodes);

        std::function<void(const SkeletonNode*)> bindNode = [&](const SkeletonNode* node) {
            auto& cache = animator->animCache[node->nodeIndex];

            auto itChannel = animator->currentAnimation->channels.find(node->name);
            if (itChannel != animator->currentAnimation->channels.end()) {
                cache.channel = &itChannel->second;
            }
            else {
                cache.channel = nullptr;
            }

            auto itBone = animator->currentSkeleton->boneMap.find(node->name);
            if (itBone != animator->currentSkeleton->boneMap.end()) {
                cache.isBone = true;
                cache.boneID = itBone->second.id;
                cache.offset = itBone->second.offset;
            }
            else {
                cache.isBone = false;
            }

            for (const auto& child : node->children) {
                bindNode(&child);
            }
            };

        bindNode(&animator->currentSkeleton->rootNode);
    }
};

#endif