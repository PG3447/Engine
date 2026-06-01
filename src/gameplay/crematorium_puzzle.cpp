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

            int targetLvl = configLeftWall[r][c];
            bool interactable = (targetLvl > 0);

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
            collider->halfSize = glm::vec3(coffinDimensions.x * 0.8f, coffinDimensions.y * 0.8f, coffinDimensions.z * 1.0f);
            collider->offset = glm::vec3(0.0f, 0.0f, coffinDimensions.z * 0.5f * w1_extendDirZ);

            CoffinData data;
            data.gameObject = obj;
            data.wall = WallSide::Left;
            data.row = r;
            data.col = c;
            data.isInteractable = interactable;
            data.preDeterminedLevel = targetLvl;
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

            int targetLvl = configRightWall[r][c];
            bool interactable = (targetLvl > 0);

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
            collider->halfSize = glm::vec3(coffinDimensions.z * 1.0f, coffinDimensions.y * 0.8f, coffinDimensions.x * 0.8f);
            collider->offset = glm::vec3(coffinDimensions.z * 0.5f * w2_extendDirX, 0.0f, 0.0f);

            CoffinData data;
            data.gameObject = obj;
            data.wall = WallSide::Right;
            data.row = r;
            data.col = c;
            data.isInteractable = interactable;
            data.preDeterminedLevel = targetLvl;
            data.basePosition = pos;
            coffins.push_back(data);
        }
    }
}

void CrematoriumPuzzle::ToggleCoffin(GameObject* clickedObject)
{
    if (!clickedObject) return;

    CoffinData* clickedData = nullptr;
    for (auto& coffin : coffins) {
        if (coffin.gameObject == clickedObject) {
            clickedData = &coffin;
            break;
        }
    }

    if (!clickedData || !clickedData->isInteractable) return;

    if (clickedData->isActivated) {
        clickedData->isActivated = false;
        clickedData->isBouncingBack = false;
        clickedData->currentTargetLevel = 0;
    }
    else {
        clickedData->isActivated = true;

        std::vector<std::vector<std::vector<bool>>> gridOccupied(
            rows, std::vector<std::vector<bool>>(cols, std::vector<bool>(cols, false))
        );

        for (auto& c : coffins) {
            if (c.isActivated && !c.isBouncingBack && &c != clickedData) {
                int r = c.row;
                for (int step = 0; step < c.currentTargetLevel; ++step) {
                    int markX = (c.wall == WallSide::Left) ? c.col : step;
                    int markZ = (c.wall == WallSide::Right) ? c.col : step;
                    gridOccupied[r][markX][markZ] = true;
                }
            }
        }

        int r = clickedData->row;
        int maxAllowedLevel = cols;

        for (int step = 0; step < cols; ++step) {
            int checkX = (clickedData->wall == WallSide::Left) ? clickedData->col : step;
            int checkZ = (clickedData->wall == WallSide::Right) ? clickedData->col : step;

            if (gridOccupied[r][checkX][checkZ]) {
                maxAllowedLevel = step;
                break;
            }
        }

        if (maxAllowedLevel >= clickedData->preDeterminedLevel) {
            clickedData->currentTargetLevel = clickedData->preDeterminedLevel;
            clickedData->isBouncingBack = false;
        }
        else {
            clickedData->currentTargetLevel = maxAllowedLevel;
            clickedData->isBouncingBack = true;
        }
    }
}

void CrematoriumPuzzle::Update(float deltaTime)
{
    float animSpeed = 60.0f;

    for (auto& coffin : coffins) {
        if (!coffin.gameObject) continue;

        auto* transform = coffin.gameObject->GetComponent<TransformComponent>();

        if (coffin.isActivated) {
            float targetDistance = 0.0f;

            if (coffin.currentTargetLevel > 0) {
                float t = static_cast<float>(coffin.currentTargetLevel - 1) / static_cast<float>(cols - 1);
                targetDistance = minExtensionDistance + t * (maxExtensionDistance - minExtensionDistance);
            }
            else {
                targetDistance = minExtensionDistance * 0.4f;
            }

            if (coffin.currentExtensionAnim < targetDistance) {
                coffin.currentExtensionAnim += animSpeed * deltaTime;

                if (coffin.currentExtensionAnim >= targetDistance) {
                    coffin.currentExtensionAnim = targetDistance;

                    if (coffin.isBouncingBack) {
                        coffin.isActivated = false;
                        coffin.isBouncingBack = false;
                        coffin.currentTargetLevel = 0;
                    }
                }
            }
        }
        else {
            // Gdy nie jest aktywna (albo w³aœnie zaczê³a wracaæ po b³êdzie), zwijamy do zera
            if (coffin.currentExtensionAnim > 0.0f) {
                coffin.currentExtensionAnim -= animSpeed * deltaTime;
                if (coffin.currentExtensionAnim < 0.0f)
                    coffin.currentExtensionAnim = 0.0f;
            }
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