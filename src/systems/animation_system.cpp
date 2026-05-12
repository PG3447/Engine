#include "animation_system.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <execution>
#include <algorithm>
#include <chrono>

AnimationSystem::AnimationSystem(ECS& ecs) {
    query = ecs.CreateQuery<AnimatorComponent>();
}

void AnimationSystem::OnGameObjectUpdated(GameObject* e) {
    query->OnGameObjectUpdated(e);
}

void AnimationSystem::Update(ECS&) {
    auto& animators = std::get<0>(query->componentsVectors);

	// to samo dt co w PhysicsSystem, kiedys mozna zmienic na globalny timer czy cos
    //float dt = 1.0f / 60.0f;

    // tymczasowo lokalny deltatime taki o
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = currentTime - lastTime;
    lastTime = currentTime;

    float dt = elapsed.count();

    if (dt > 0.1f) dt = 0.016f;

    std::for_each(std::execution::par_unseq, animators.begin(), animators.end(),
        [this, dt](AnimatorComponent* animator) {

            if (!animator || !animator->currentAnimation || !animator->currentSkeleton)
                return;

            if (animator->isFinished)
                return;

            animator->currentTime += animator->currentAnimation->ticksPerSecond * dt * animator->playbackSpeed;

            if (animator->currentTime >= animator->currentAnimation->duration) {
                if (animator->looping) {
                    if (animator->currentAnimation->duration > 0.001f) {
                        animator->currentTime = fmod(animator->currentTime, animator->currentAnimation->duration);
                    }
                    else {
                        animator->currentTime = 0.0f;
                    }
                }
                else {
                    animator->currentTime = animator->currentAnimation->duration;
                    animator->isFinished = true;
                }
            }

            glm::mat4 rootTransform = glm::mat4(1.0f);

            this->CalculateBoneTransform(&animator->currentSkeleton->rootNode, rootTransform, animator);
        }
    );
}

void AnimationSystem::CalculateBoneTransform(const SkeletonNode* node, const glm::mat4& parentTransform, AnimatorComponent* animator) {
    glm::mat4 nodeTransform = node->localTransform;

    if (node->nodeIndex < animator->animCache.size()) {
        auto& cache = animator->animCache[node->nodeIndex];

        if (cache.channel) {
            const auto& channel = *cache.channel;

            glm::vec3 scale = node->defaultScale;
            glm::vec3 pos = node->defaultPosition;
            if (!channel.positions.empty()) {
                pos = CalcInterpolatedPosition(animator->currentTime, channel, cache.lastPosIndex);
            }

            glm::quat rot = channel.rotations.empty() ?
                node->defaultRotation :
                CalcInterpolatedRotation(animator->currentTime, channel, cache.lastRotIndex);

            if (!channel.scales.empty()) {
                scale = CalcInterpolatedScale(animator->currentTime, channel, cache.lastScaleIndex);
            }

            glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), pos);
            glm::mat4 rotationMat = glm::toMat4(rot);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

            nodeTransform = translationMat * rotationMat * scaleMat;
        }

        glm::mat4 globalTransform = parentTransform * nodeTransform;

        if (cache.isBone) {
            animator->finalBoneMatrices[cache.boneID] = animator->currentSkeleton->globalInverseTransform * globalTransform * cache.offset;
        }

        for (const auto& child : node->children) {
            CalculateBoneTransform(&child, globalTransform, animator);
        }
    }
}

int AnimationSystem::GetPositionIndex(float animationTime, const AnimationChannel& channel, int& lastIndex) {
    if (lastIndex >= channel.positions.size() - 1 || animationTime < channel.positions[lastIndex].timeStamp) {
        lastIndex = 0;
    }

    for (int i = lastIndex; i < (int)channel.positions.size() - 1; ++i) {
        if (animationTime < channel.positions[i + 1].timeStamp) {
            lastIndex = i;
            return i;
        }
    }
    return (int)channel.positions.size() - 2;
}

int AnimationSystem::GetRotationIndex(float animationTime, const AnimationChannel& channel, int& lastIndex) {
    if (lastIndex >= channel.rotations.size() - 1 || animationTime < channel.rotations[lastIndex].timeStamp) {
        lastIndex = 0;
    }

    for (int i = lastIndex; i < (int)channel.rotations.size() - 1; ++i) {
        if (animationTime < channel.rotations[i + 1].timeStamp) {
            lastIndex = i;
            return i;
        }
    }
    return (int)channel.rotations.size() - 2;
}

int AnimationSystem::GetScaleIndex(float animationTime, const AnimationChannel& channel, int& lastIndex) {
    if (lastIndex >= channel.scales.size() - 1 || animationTime < channel.scales[lastIndex].timeStamp) {
        lastIndex = 0;
    }

    for (int i = lastIndex; i < (int)channel.scales.size() - 1; ++i) {
        if (animationTime < channel.scales[i + 1].timeStamp) {
            lastIndex = i;
            return i;
        }
    }
    return (int)channel.scales.size() - 2;
}

glm::vec3 AnimationSystem::CalcInterpolatedPosition(float animationTime, const AnimationChannel& channel, int& lastIndex) {
    if (channel.positions.empty()) return glm::vec3(0.0f);
    if (channel.positions.size() == 1) return channel.positions[0].position;

    int p0Index = GetPositionIndex(animationTime, channel, lastIndex);
    int p1Index = p0Index + 1;
    float delta = channel.positions[p1Index].timeStamp - channel.positions[p0Index].timeStamp;
    float scaleFactor = 0.0f;
    if (delta > 0.00001f) {
        scaleFactor = (animationTime - channel.positions[p0Index].timeStamp) / delta;
    }
    scaleFactor = glm::clamp(scaleFactor, 0.0f, 1.0f);
    return glm::mix(channel.positions[p0Index].position, channel.positions[p1Index].position, scaleFactor);
}

glm::quat AnimationSystem::CalcInterpolatedRotation(float animationTime, const AnimationChannel& channel, int& lastIndex) {
    if (channel.rotations.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (channel.rotations.size() == 1) return glm::normalize(channel.rotations[0].orientation);

    int p0Index = GetRotationIndex(animationTime, channel, lastIndex);
    int p1Index = p0Index + 1;
    float delta = channel.rotations[p1Index].timeStamp - channel.rotations[p0Index].timeStamp;
    float scaleFactor = 0.0f;
    if (delta > 0.00001f) {
        scaleFactor = (animationTime - channel.rotations[p0Index].timeStamp) / delta;
    }
    scaleFactor = glm::clamp(scaleFactor, 0.0f, 1.0f);
    glm::quat finalRot = glm::slerp(channel.rotations[p0Index].orientation, channel.rotations[p1Index].orientation, scaleFactor);
    return glm::normalize(finalRot);
}

glm::vec3 AnimationSystem::CalcInterpolatedScale(float animationTime, const AnimationChannel& channel, int& lastIndex) {
    if (channel.scales.empty()) return glm::vec3(1.0f);
    if (channel.scales.size() == 1) return channel.scales[0].scale;

    int p0Index = GetScaleIndex(animationTime, channel, lastIndex);
    int p1Index = p0Index + 1;
    float delta = channel.scales[p1Index].timeStamp - channel.scales[p0Index].timeStamp;
    float scaleFactor = 0.0f;
    if (delta > 0.00001f) {
        scaleFactor = (animationTime - channel.scales[p0Index].timeStamp) / delta;
    }
    scaleFactor = glm::clamp(scaleFactor, 0.0f, 1.0f);
    return glm::mix(channel.scales[p0Index].scale, channel.scales[p1Index].scale, scaleFactor);
}