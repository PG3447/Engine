#ifndef ANIMATION_CLIP_H
#define ANIMATION_CLIP_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct KeyPosition
{
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation
{
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale
{
    glm::vec3 scale;
    float timeStamp;
};

struct AnimationChannel
{
    std::string boneName;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
};

class AnimationClip
{
public:
    std::string name;

    float duration;

    float ticksPerSecond;

    std::unordered_map<std::string, AnimationChannel> channels;

    AnimationClip() = default;
    ~AnimationClip() = default;

    bool HasChannel(const std::string& boneName) const
    {
        return channels.find(boneName) != channels.end();
    }
};

#endif