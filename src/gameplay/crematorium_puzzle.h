#pragma once
#include "core/scene.h"
#include "model.h"
#include "prefab.h"
#include "shader.h"
#include "resource_manager.h"
#include <vector>
#include <algorithm>
#include <systems/AudioSystem.h>

enum class WallSide {
    Left,
    Right
};

struct CoffinData {
    GameObject* gameObject = nullptr;
    TransformComponent* transform = nullptr;
    WallSide wall;
    int row;
    int col;

    bool isInteractable = true;
    bool isActivated = false;

    int preDeterminedLevel = 0;
    bool isBouncingBack = false;

    int currentTargetLevel = 0;
    float currentExtensionAnim = 0.0f;

    glm::vec3 basePosition;

    FMOD::Channel* slidingChannel = nullptr;
};

class CrematoriumPuzzle {
public:
    int rows = 4;
    int cols = 4;

    std::vector<std::vector<int>> configLeftWall = {
        {4, 4, 3, 1},
        {2, 3, 2, 1},
        {4, 4, 0, 2},
        {1, 4, 2, 1}
    };

    std::vector<std::vector<int>> configRightWall = {
        {3, 2, 4, 0},
        {4, 3, 2, 1},
        {1, 2, 3, 3},
        {3, 1, 3, 4}
    };

    std::vector<std::vector<int>> corpsesLeftWall = {
        {0, 1, 0, 0},
        {1, 0, 0, 1},
        {0, 1, 1, 0},
        {0, 0, 0, 1}
    };

    std::vector<std::vector<int>> corpsesRightWall = {
        {1, 0, 1, 0},
        {0, 0, 1, 0},
        {1, 1, 0, 0},
        {0, 0, 0, 1}
    };

    //int rows = 5;
    //int cols = 5;

    //std::vector<std::vector<int>> configLeftWall = {
    //    {3, 3, 2, 5, 0},
    //    {5, 5, 4, 2, 1},
    //    {2, 1, 4, 3, 2},
    //    {4, 0, 5, 0, 0},
    //    {4, 2, 3, 4, 3}
    //};

    //std::vector<std::vector<int>> configRightWall = {
    //    {2, 3, 3, 5, 2},
    //    {2, 0, 2, 5, 5},
    //    {1, 3, 2, 0, 4},
    //    {4, 4, 2, 5, 4},
    //    {0, 2, 0, 5, 1}
    //};

    AudioSystem* audioSystem = nullptr;
    FMOD::Sound* soundSlideOut = nullptr;
    FMOD::Sound* soundSlideIn = nullptr;
    FMOD::Sound* soundCollide = nullptr;
    FMOD::Sound* soundClose = nullptr;
    FMOD::Sound* soundPuzzleSolved = nullptr;

    //std::pair<int, int> leftStart = { 0, 0 };
    //std::pair<int, int> leftEnd = { 4, 4 };

    //std::pair<int, int> rightStart = { 4, 4 };
    //std::pair<int, int> rightEnd = { 0, 0 };

    std::pair<int, int> leftStart = { 0, 0 };
    std::pair<int, int> leftEnd = { 3, 3 };

    std::pair<int, int> rightStart = { 3, 3 };
    std::pair<int, int> rightEnd = { 0, 0 };

    std::unordered_map<GameObject*, std::shared_ptr<Material>> activeMaterials;
    std::unordered_map<GameObject*, std::shared_ptr<Material>> inactiveMaterials;

    bool isLeftSolved = false;
    bool isRightSolved = false;
    bool isPuzzleSolved = false;

    float spacingHorizontal = 6.0f;
    float spacingVertical = 2.5f;

    float minExtensionDistance = 12.0f;
    float maxExtensionDistance = 40.5f;

    float wallOffset = 16.0f;

    glm::vec3 renderScale = glm::vec3(1.0f, 1.0f, 1.0f);

    // glownie dla collidera
    glm::vec3 coffinDimensions = glm::vec3(1.0f, 1.0f, 28.0f);

    float w1_buildDirX = -1.0f;
    float w1_extendDirZ = 1.0f;
    float w2_buildDirZ = 1.0f;
    float w2_extendDirX = -1.0f;

    GameObject* leftPanelObj = nullptr;
    GameObject* rightPanelObj = nullptr;

    std::vector<CoffinData> coffins;
    uint64_t activationCounter = 1;

    void Init(Scene* scene, Prefab* redEmpty, Prefab* redCorpse, Prefab* greenEmpty, Prefab* greenCorpse, Prefab* baseEmpty, Prefab* baseCorpse, Prefab* panelPrefab, Shader* shader, glm::vec3 cornerPosition);
    void SetupAudio(AudioSystem* audioSys, FMOD::Sound* slideOut, FMOD::Sound* slideIn, FMOD::Sound* collide, FMOD::Sound* close, FMOD::Sound* puzzleSolved = nullptr);
    void Update(float deltaTime);
    void ToggleCoffin(GameObject* clickedObject);
};