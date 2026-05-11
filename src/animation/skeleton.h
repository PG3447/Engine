#ifndef SKELETON_H
#define SKELETON_H

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "bone_info.h"

struct SkeletonNode
{
    std::string name;
    int nodeIndex;

    glm::mat4 localTransform;

    glm::vec3 defaultPosition;
    glm::quat defaultRotation;
    glm::vec3 defaultScale;

    std::vector<SkeletonNode> children;
};

class Skeleton
{
public:
    SkeletonNode rootNode;

    std::unordered_map<std::string, BoneInfo> boneMap;

    int boneCount = 0;
    int totalNodes = 0;

    glm::mat4 globalInverseTransform = glm::mat4(1.0f);

    Skeleton() = default;
    ~Skeleton() = default;

    bool HasBone(const std::string& name) const
    {
        return boneMap.find(name) != boneMap.end();
    }
};

#endif