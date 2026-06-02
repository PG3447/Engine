#include "crematorium_puzzle.h"

void CrematoriumPuzzle::Init(Scene* scene, std::shared_ptr<Model> coffinModel, Prefab* panelPrefab, Shader* shader, glm::vec3 cornerPosition)
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

            int configRow = (rows - 1) - r;
            int configCol = (cols - 1) - c;

            int targetLvl = configLeftWall[configRow][configCol];
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

            int configRow = (rows - 1) - r;
            int configCol = c;

            int targetLvl = configRightWall[configRow][configCol];
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

    leftPanelObj = panelPrefab->Instantiate(*scene, puzzleRoot, shader);
    leftPanelObj->name = "Panel_Left";

    auto* tLeft = leftPanelObj->GetComponent<TransformComponent>();
    //tLeft->position = cornerPosition + glm::vec3(-15.0f, 20.0f, -5.0f);
    tLeft->position = glm::vec3(130.0f, 10.15f, -260.34f);
    tLeft->scale = glm::vec3(1.0f);
    tLeft->rotation = glm::vec3(0.0f, -90.0f, 0.0f);
    tLeft->isDirty = true;

    leftPanelObj->TraverseChildren([](GameObject* child) {
        auto* render = child->GetComponent<RenderComponent>();
        if (render) {
            for (auto& mesh : render->meshes) {
                if (mesh.material) {
                    mesh.material = std::make_shared<Material>(*mesh.material);
                    mesh.material->diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
                }
            }
        }
    });

    rightPanelObj = panelPrefab->Instantiate(*scene, puzzleRoot, shader);
    rightPanelObj->name = "Panel_Right";

    auto* tRight = rightPanelObj->GetComponent<TransformComponent>();
    //tRight->position = cornerPosition + glm::vec3(-5.0f, 20.0f, -15.0f);
    tRight->position = glm::vec3(180.66f, 10.77f, -210.87f);
    tRight->scale = glm::vec3(1.0f);
    tRight->rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    tRight->isDirty = true;

    rightPanelObj->TraverseChildren([](GameObject* child) {
        auto* render = child->GetComponent<RenderComponent>();
        if (render) {
            for (auto& mesh : render->meshes) {
                if (mesh.material) {
                    mesh.material = std::make_shared<Material>(*mesh.material);
                    mesh.material->diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
                }
            }
        }
    });

    auto setupPanelMaterials = [&](GameObject* panelObj, glm::vec3 activeColor) {
        if (!panelObj) return;
        panelObj->TraverseChildren([&](GameObject* child) {
            auto* render = child->GetComponent<RenderComponent>();
            if (render) {
                std::string objName = child->name;
                std::string parentName = child->GetParent() ? child->GetParent()->name : "";

                bool isEndObject = (objName.find("End") != std::string::npos) || (parentName.find("End") != std::string::npos);

                for (auto& mesh : render->meshes) {
                    if (mesh.material) {
                        auto inactiveMat = std::make_shared<Material>(*mesh.material);
                        inactiveMat->diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
                        inactiveMaterials[child] = inactiveMat;

                        auto activeMat = std::make_shared<Material>(*mesh.material);
                        activeMat->diffuseColor = activeColor;
                        activeMaterials[child] = activeMat;

                        if (isEndObject) {
                            mesh.material = activeMat;
                        }
                        else {
                            mesh.material = inactiveMat;
                        }
                    }
                }
                child->NotifyChanged();
            }
            });
        };

    setupPanelMaterials(leftPanelObj, glm::vec3(1.0f, 0.0f, 0.0f));
    setupPanelMaterials(rightPanelObj, glm::vec3(0.0f, 1.0f, 0.0f));
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

    auto updatePanelColors = [&](GameObject* panel, WallSide side, glm::vec3 activeColor) {
        if (!panel) return;

        std::vector<std::vector<bool>> grid(rows, std::vector<bool>(cols, false));

        for (auto& coffin : coffins) {
            if (coffin.wall == side && coffin.isActivated && !coffin.isBouncingBack) {
                int mappedRow = (rows - 1) - coffin.row;
                int mappedCol = (side == WallSide::Right) ? coffin.col : ((cols - 1) - coffin.col);

                grid[mappedRow][mappedCol] = true;
            }
        }

        panel->TraverseChildren([&](GameObject* child) {
            auto* render = child->GetComponent<RenderComponent>();
            if (!render) return;

            std::string objName = child->name;
            std::string parentName = child->GetParent() ? child->GetParent()->name : "";

            bool shouldBeActive = false;
            bool isEndObject = (objName.find("End") != std::string::npos) || (parentName.find("End") != std::string::npos);

            if (isEndObject) {
                shouldBeActive = true;
            }
            else {
                bool isCable = (objName.find("->") != std::string::npos) || (parentName.find("->") != std::string::npos);

                if (isCable) {
                    for (int r = 0; r < rows; ++r) {
                        for (int c = 0; c < cols; ++c) {
                            std::string currentCube = std::to_string(r + 1) + "x" + std::to_string(c + 1);

                            if (c < cols - 1) {
                                std::string cableName = currentCube + "->" + std::to_string(r + 1) + "x" + std::to_string(c + 2);
                                if (objName.find(cableName) != std::string::npos || parentName.find(cableName) != std::string::npos) {
                                    if (grid[r][c] && grid[r][c + 1]) shouldBeActive = true;
                                }
                            }
                            if (r < rows - 1) {
                                std::string cableName = currentCube + "->" + std::to_string(r + 2) + "x" + std::to_string(c + 1);
                                if (objName.find(cableName) != std::string::npos || parentName.find(cableName) != std::string::npos) {
                                    if (grid[r][c] && grid[r + 1][c]) shouldBeActive = true;
                                }
                            }
                        }
                    }
                }
                else {
                    for (int r = 0; r < rows; ++r) {
                        for (int c = 0; c < cols; ++c) {
                            std::string cubeName = std::to_string(r + 1) + "x" + std::to_string(c + 1);
                            if (objName.find(cubeName) != std::string::npos || parentName.find(cubeName) != std::string::npos) {
                                if (grid[r][c]) shouldBeActive = true;
                            }
                        }
                    }
                }
            }

            bool wasChanged = false;
            for (auto& mesh : render->meshes) {
                std::shared_ptr<Material> targetMat = shouldBeActive ? activeMaterials[child] : inactiveMaterials[child];
                if (mesh.material != targetMat) {
                    mesh.material = targetMat;
                    wasChanged = true;
                }
            }

            if (wasChanged) {
                child->NotifyChanged();
            }
            });
        };

    updatePanelColors(leftPanelObj, WallSide::Left, glm::vec3(1.0f, 0.0f, 0.0f));
    updatePanelColors(rightPanelObj, WallSide::Right, glm::vec3(0.0f, 1.0f, 0.0f));

    std::vector<std::vector<bool>> gridLeft(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<bool>> gridRight(rows, std::vector<bool>(cols, false));

    for (auto& coffin : coffins) {
        if (coffin.isActivated && !coffin.isBouncingBack) {
            int mappedRow = (rows - 1) - coffin.row;
            int mappedCol = (coffin.wall == WallSide::Right) ? coffin.col : ((cols - 1) - coffin.col);

            if (coffin.wall == WallSide::Left) {
                gridLeft[mappedRow][mappedCol] = true;
            }
            else if (coffin.wall == WallSide::Right) {
                gridRight[mappedRow][mappedCol] = true;
            }
        }
    }

    auto hasPath = [&](const std::vector<std::vector<bool>>& grid, std::pair<int, int> start, std::pair<int, int> end) -> bool {
        if (!grid[start.first][start.second] || !grid[end.first][end.second]) return false;

        std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
        std::vector<std::pair<int, int>> stack;

        stack.push_back(start);
        visited[start.first][start.second] = true;

        int dr[] = { -1, 1, 0, 0 };
        int dc[] = { 0, 0, -1, 1 };

        while (!stack.empty()) {
            auto curr = stack.back();
            stack.pop_back();

            if (curr.first == end.first && curr.second == end.second) return true;

            for (int i = 0; i < 4; ++i) {
                int nr = curr.first + dr[i];
                int nc = curr.second + dc[i];

                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                    if (grid[nr][nc] && !visited[nr][nc]) {
                        visited[nr][nc] = true;
                        stack.push_back({ nr, nc });
                    }
                }
            }
        }
        return false;
        };

    bool currentLeftSolved = hasPath(gridLeft, leftStart, leftEnd);
    bool currentRightSolved = hasPath(gridRight, rightStart, rightEnd);

    if (currentLeftSolved && !isLeftSolved) {
        spdlog::info("Lewa Sciana Zostala Polaczona");
        isLeftSolved = true;
    }
    else if (!currentLeftSolved && isLeftSolved) {
        isLeftSolved = false;
    }

    if (currentRightSolved && !isRightSolved) {
        spdlog::info("Prawa Sciana Zostala Polaczona");
        isRightSolved = true;
    }
    else if (!currentRightSolved && isRightSolved) {
        isRightSolved = false;
    }

    if (isLeftSolved && isRightSolved && !isPuzzleSolved) {
        spdlog::warn("ZAGADKA KREMATORIUM ZOSTALA ROZWIAZANA W PELNI WOOOOOOOOOOOW");
        isPuzzleSolved = true;
    }
    else if ((!isLeftSolved || !isRightSolved) && isPuzzleSolved) {
        isPuzzleSolved = false;
    }
}