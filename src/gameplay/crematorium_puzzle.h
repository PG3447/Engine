#pragma once
#include "core/scene.h"
#include "model.h"
#include <vector>
#include <algorithm>

enum class WallSide {
    Left,
    Right
};

struct CoffinData {
    GameObject* gameObject = nullptr;
    WallSide wall;
    int row;
    int col;

    bool isInteractable = true;
    bool isActivated = false;
    uint64_t activationOrder = 0;

    int currentTargetLevel = 0;
    float currentExtensionAnim = 0.0f;

    glm::vec3 basePosition;
};

class CrematoriumPuzzle {
public:
    int rows = 4;
    int cols = 6;

    // 1 - kolor, 0 - czarne
    std::vector<std::vector<int>> configLeftWall = {
        {1, 0, 1, 1, 0, 1}, // najnizszy
        {0, 1, 1, 0, 1, 1},
        {1, 1, 0, 1, 1, 0},
        {1, 1, 1, 1, 0, 0} // najwyzszy
    };

    // 1 - kolor, 0 - czarne
    std::vector<std::vector<int>> configRightWall = {
        {0, 1, 0, 1, 1, 0},
        {1, 0, 1, 1, 0, 1},
        {1, 1, 1, 0, 1, 1},
        {0, 1, 1, 1, 0, 1}
    };

    float spacingHorizontal = 6.0f;
    float spacingVertical = 3.5f;

    float minExtensionDistance = 12.0f;
    float maxExtensionDistance = 40.5f;

    float wallOffset = 18.0f;

    glm::vec3 coffinScale = glm::vec3(1.55f, 1.05f, 32.0f);

    float w1_buildDirX = -1.0f;
    float w1_extendDirZ = 1.0f;
    float w2_buildDirZ = 1.0f;
    float w2_extendDirX = -1.0f;

    std::vector<CoffinData> coffins;
    uint64_t activationCounter = 1;

    void Init(Scene* scene, std::shared_ptr<Model> coffinModel, glm::vec3 cornerPosition);
    void Update(float deltaTime);
    void ToggleCoffin(GameObject* clickedObject);
};