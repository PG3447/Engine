#include "crematorium_puzzle.h"

void CrematoriumPuzzle::Init(Scene* scene, std::shared_ptr<Model> coffinModel, glm::vec3 cornerPosition)
{
    GameObject* puzzleRoot = scene->CreateGameObject();
    puzzleRoot->name = "Crematorium_Puzzle_Root";

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            GameObject* obj = scene->CreateGameObject(puzzleRoot);
            obj->name = "Coffin_L_" + std::to_string(r) + "_" + std::to_string(c);

            auto* transform = obj->AddComponent<TransformComponent>();
            transform->scale = renderScale;

            glm::vec3 posOnWall = cornerPosition + glm::vec3((c + 1) * spacingHorizontal * w1_buildDirX, r * spacingVertical, 0.0f);
            glm::vec3 pos = posOnWall - glm::vec3(0.0f, 0.0f, (coffinDimensions.z + wallOffset) * w1_extendDirZ);
            transform->position = pos;

            bool interactable = (configLeftWall[r][c] == 1);

            glm::vec3 coffinColor = interactable ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.1f, 0.1f, 0.1f);

            auto* render = obj->AddComponent<RenderComponent>();
            render->meshes = coffinModel->rootNode->meshes;
            for (auto& mesh : render->meshes) {
                if (mesh.material) {
                    mesh.material = std::make_shared<Material>(*mesh.material);
                    mesh.material->diffuseColor = coffinColor;
                }
            }

            auto* collider = obj->AddComponent<ColliderComponent>();
            collider->halfSize = glm::vec3(coffinDimensions.x * 0.8f, coffinDimensions.y * 0.8f, coffinDimensions.z * 1.2f);
            collider->offset = glm::vec3(0.0f, 0.0f, coffinDimensions.z * 0.5f * w1_extendDirZ);

            CoffinData data;
            data.gameObject = obj;
            data.wall = WallSide::Left;
            data.row = r;
            data.col = c;
            data.isInteractable = interactable;
            data.basePosition = pos;
            coffins.push_back(data);
        }
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            GameObject* obj = scene->CreateGameObject(puzzleRoot);
            obj->name = "Coffin_R_" + std::to_string(r) + "_" + std::to_string(c);

            auto* transform = obj->AddComponent<TransformComponent>();
            transform->scale = renderScale;

            glm::vec3 posOnWall = cornerPosition + glm::vec3(0.0f, r * spacingVertical, (c + 1) * spacingHorizontal * w2_buildDirZ);
            glm::vec3 pos = posOnWall - glm::vec3((coffinDimensions.z + wallOffset) * w2_extendDirX, 0.0f, 0.0f);
            transform->position = pos;
            transform->rotation = glm::vec3(0.0f, -90.0f, 0.0f);

            bool interactable = (configRightWall[r][c] == 1);

            glm::vec3 coffinColor = interactable ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.1f, 0.1f, 0.1f);

            auto* render = obj->AddComponent<RenderComponent>();
            render->meshes = coffinModel->rootNode->meshes;
            for (auto& mesh : render->meshes) {
                if (mesh.material) {
                    mesh.material = std::make_shared<Material>(*mesh.material);
                    mesh.material->diffuseColor = coffinColor;
                }
            }

            auto* collider = obj->AddComponent<ColliderComponent>();
            collider->halfSize = glm::vec3(coffinDimensions.z * 1.2f, coffinDimensions.y * 0.8f, coffinDimensions.x * 0.8f);
            collider->offset = glm::vec3(coffinDimensions.z * 0.5f * w2_extendDirX, 0.0f, 0.0f);

            CoffinData data;
            data.gameObject = obj;
            data.wall = WallSide::Right;
            data.row = r;
            data.col = c;
            data.isInteractable = interactable;
            data.basePosition = pos;
            coffins.push_back(data);
        }
    }
}

void CrematoriumPuzzle::ToggleCoffin(GameObject* clickedObject)
{
    if (!clickedObject) return;
    bool stateChanged = false;

    for (auto& coffin : coffins) {
        if (coffin.gameObject == clickedObject) {

            if (!coffin.isInteractable) return;

            coffin.isActivated = !coffin.isActivated;
            if (coffin.isActivated) {
                coffin.activationOrder = activationCounter++;
            }
            else {
                coffin.activationOrder = 0;
                coffin.currentTargetLevel = 0;
            }
            stateChanged = true;
            break;
        }
    }

    if (stateChanged) {
        std::vector<CoffinData*> activeCoffins;
        for (auto& coffin : coffins) {
            if (coffin.isActivated) activeCoffins.push_back(&coffin);
        }

        std::sort(activeCoffins.begin(), activeCoffins.end(), [](CoffinData* a, CoffinData* b) {
            return a->activationOrder < b->activationOrder;
            });

        std::vector<std::vector<std::vector<bool>>> gridOccupied(
            rows, std::vector<std::vector<bool>>(cols, std::vector<bool>(cols, false))
        );

        for (auto* coffin : activeCoffins) {
            int r = coffin->row;
            int maxAllowedLevel = cols;

            for (int step = 0; step < cols; ++step) {
                int checkX = (coffin->wall == WallSide::Left) ? coffin->col : step;
                int checkZ = (coffin->wall == WallSide::Right) ? coffin->col : step;

                if (gridOccupied[r][checkX][checkZ]) {
                    maxAllowedLevel = step;
                    break;
                }
            }

            coffin->currentTargetLevel = maxAllowedLevel;

            for (int step = 0; step < maxAllowedLevel; ++step) {
                int markX = (coffin->wall == WallSide::Left) ? coffin->col : step;
                int markZ = (coffin->wall == WallSide::Right) ? coffin->col : step;
                gridOccupied[r][markX][markZ] = true;
            }
        }
    }
}

void CrematoriumPuzzle::Update(float deltaTime)
{
    float animSpeed = 60.0f;

    for (auto& coffin : coffins) {
        if (!coffin.gameObject) continue;

        auto* transform = coffin.gameObject->GetComponent<TransformComponent>();
        auto* render = coffin.gameObject->GetComponent<RenderComponent>();

        float targetDistance = 0.0f;

        if (coffin.currentTargetLevel > 0) {
            float t = static_cast<float>(coffin.currentTargetLevel - 1) / static_cast<float>(cols - 1);

            targetDistance = minExtensionDistance + t * (maxExtensionDistance - minExtensionDistance);
        }

        if (coffin.currentExtensionAnim < targetDistance) {
            coffin.currentExtensionAnim += animSpeed * deltaTime;
            if (coffin.currentExtensionAnim > targetDistance)
                coffin.currentExtensionAnim = targetDistance;
        }
        else if (coffin.currentExtensionAnim > targetDistance) {
            coffin.currentExtensionAnim -= animSpeed * deltaTime;
            if (coffin.currentExtensionAnim < targetDistance)
                coffin.currentExtensionAnim = targetDistance;
        }

        float extensionDistance = coffin.currentExtensionAnim;

        if (coffin.wall == WallSide::Left) {
            transform->position = coffin.basePosition + glm::vec3(0.0f, 0.0f, extensionDistance * w1_extendDirZ);
        }
        else {
            transform->position = coffin.basePosition + glm::vec3(extensionDistance * w2_extendDirX, 0.0f, 0.0f);
        }
        transform->isDirty = true;
    }
}