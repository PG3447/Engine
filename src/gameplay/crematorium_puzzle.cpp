#include "crematorium_puzzle.h"

void CrematoriumPuzzle::Init(Scene* scene, Prefab* redEmpty, Prefab* redCorpse, Prefab* greenEmpty, Prefab* greenCorpse, Prefab* baseEmpty, Prefab* baseCorpse, Prefab* panelPrefab, Shader* shader, glm::vec3 cornerPosition)
{
    GameObject* puzzleRoot = scene->CreateGameObject();
    puzzleRoot->name = "Crematorium_Puzzle_Root";

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int configRow = (rows - 1) - r;
            int configCol = (cols - 1) - c;
            int targetLvl = configLeftWall[configRow][configCol];
            bool interactable = (targetLvl > 0);

            Prefab* prefabToUse;
            if (targetLvl == 0) {
                prefabToUse = (corpsesLeftWall[configRow][configCol] == 1) ? baseCorpse : baseEmpty;
            }
            else {
                prefabToUse = (corpsesLeftWall[configRow][configCol] == 1) ? redCorpse : redEmpty;
            }

            GameObject* obj = scene->CreateGameObject(puzzleRoot);
            obj->name = "Coffin_L_" + std::to_string(r) + "_" + std::to_string(c);

            auto* transform = obj->AddComponent<TransformComponent>();
            transform->scale = renderScale;

            glm::vec3 posOnWall = cornerPosition + glm::vec3((c + 1) * spacingHorizontal * w1_buildDirX, r * spacingVertical, 0.0f);
            glm::vec3 pos = posOnWall - glm::vec3(0.0f, 0.0f, wallOffset * w1_extendDirZ);
            transform->position = pos;

            auto* render = obj->AddComponent<RenderComponent>();
            if (prefabToUse->rootModel && prefabToUse->rootModel->rootNode) {
                auto gatherMeshes = [](auto& self, ModelNode* node, RenderComponent* rc) -> void {
                    if (!node) return;
                    for (auto& mesh : node->meshes) {
                        rc->meshes.push_back(mesh);
                    }
                    for (auto& child : node->children) {
                        self(self, child.get(), rc);
                    }
                    };
                gatherMeshes(gatherMeshes, prefabToUse->rootModel->rootNode.get(), render);
            }

            auto* collider = obj->AddComponent<ColliderComponent>();
            collider->halfSize = glm::vec3(coffinDimensions.x * 0.5f, coffinDimensions.y * 0.5f, coffinDimensions.z * 0.5f);

            collider->offset = glm::vec3(0.0f, 0.0f, -coffinDimensions.z * 0.5f * w1_extendDirZ);

            CoffinData data;
            data.gameObject = obj;
            data.transform = transform;
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
            int configRow = (rows - 1) - r;
            int configCol = c;
            int targetLvl = configRightWall[configRow][configCol];
            bool interactable = (targetLvl > 0);

            Prefab* prefabToUse;
            if (targetLvl == 0) {
                prefabToUse = (corpsesRightWall[configRow][configCol] == 1) ? baseCorpse : baseEmpty;
            }
            else {
                prefabToUse = (corpsesRightWall[configRow][configCol] == 1) ? greenCorpse : greenEmpty;
            }

            GameObject* obj = scene->CreateGameObject(puzzleRoot);
            obj->name = "Coffin_R_" + std::to_string(r) + "_" + std::to_string(c);

            auto* transform = obj->AddComponent<TransformComponent>();
            transform->scale = renderScale;

            glm::vec3 posOnWall = cornerPosition + glm::vec3(0.0f, r * spacingVertical, (c + 1) * spacingHorizontal * w2_buildDirZ);
            glm::vec3 pos = posOnWall - glm::vec3(wallOffset * w2_extendDirX, 0.0f, 0.0f);
            transform->position = pos;
            transform->rotation = glm::vec3(0.0f, -90.0f, 0.0f);

            auto* render = obj->AddComponent<RenderComponent>();
            if (prefabToUse->rootModel && prefabToUse->rootModel->rootNode) {
                auto gatherMeshes = [](auto& self, ModelNode* node, RenderComponent* rc) -> void {
                    if (!node) return;
                    for (auto& mesh : node->meshes) {
                        rc->meshes.push_back(mesh);
                    }
                    for (auto& child : node->children) {
                        self(self, child.get(), rc);
                    }
                    };
                gatherMeshes(gatherMeshes, prefabToUse->rootModel->rootNode.get(), render);
            }

            auto* collider = obj->AddComponent<ColliderComponent>();
            collider->halfSize = glm::vec3(coffinDimensions.z * 0.5f, coffinDimensions.y * 0.5f, coffinDimensions.x * 0.5f);

            collider->offset = glm::vec3(-coffinDimensions.z * 0.5f * w2_extendDirX, 0.0f, 0.0f);

            CoffinData data;
            data.gameObject = obj;
            data.transform = transform;
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
    tLeft->position = cornerPosition + glm::vec3(-37.0f, 4.0f, 2.0f);
    tLeft->scale = glm::vec3(2.0f);
    tLeft->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    tLeft->isDirty = true;

    leftPanelObj->AddComponent<ColliderComponent>();
	leftPanelObj->GetComponent<ColliderComponent>()->halfSize = glm::vec3(4.0f, 5.0f, 4.0f);

    rightPanelObj = panelPrefab->Instantiate(*scene, puzzleRoot, shader);
    rightPanelObj->name = "Panel_Right";
    auto* tRight = rightPanelObj->GetComponent<TransformComponent>();
    tRight->position = cornerPosition + glm::vec3(-2.0f, 4.0f, 37.0f);
    tRight->scale = glm::vec3(2.0f);
    tRight->rotation = glm::vec3(0.0f, -90.0f, 0.0f);
    tRight->isDirty = true;

    rightPanelObj->AddComponent<ColliderComponent>();
    rightPanelObj->GetComponent<ColliderComponent>()->halfSize = glm::vec3(4.0f, 5.0f, 4.0f);

    auto setupPanelMaterials = [&](GameObject* panelObj, glm::vec3 activeColor) {
        if (!panelObj) return;
        panelObj->TraverseChildren([&](GameObject* child) {
            auto* render = child->GetComponent<RenderComponent>();
            if (render) {
                std::string objName = child->name;
                std::string parentName = child->GetParent() ? child->GetParent()->name : "";

                bool isCable = (objName.find("->") != std::string::npos) || (parentName.find("->") != std::string::npos);
                bool isEndObject = (objName.find("End") != std::string::npos) || (parentName.find("End") != std::string::npos);

                bool isPoint = false;
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        std::string cubeName = std::to_string(r + 1) + "x" + std::to_string(c + 1);
                        if (objName.find(cubeName) != std::string::npos || parentName.find(cubeName) != std::string::npos) {
                            isPoint = true;
                            break;
                        }
                    }
                    if (isPoint) break;
                }

                if (!isPoint && !isCable && !isEndObject) return;

                for (auto& mesh : render->meshes) {
                    if (mesh.material) {
                        auto createSolidColorMat = [&](glm::vec3 color) {
                            auto mat = std::make_shared<Material>(*mesh.material);
                            mat->diffuseMap = 0;
                            mat->normalMap = 0;
                            mat->aoMap = 0;
                            mat->diffuseColor = glm::vec4(color, 1.0f);
                            mat->metallicRoughnessMap = ResourceManager::CreateTextureFromColor("neon_matte_mr", glm::vec3(1.0f, 1.0f, 0.0f)).id;
                            mat->shininess = 0.0f;
                            return mat;
                            };

                        auto inactiveMat = createSolidColorMat(glm::vec3(0.1f, 0.1f, 0.1f));
                        inactiveMaterials[child] = inactiveMat;

                        auto activeMat = createSolidColorMat(activeColor);
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

    setupPanelMaterials(leftPanelObj, glm::vec3(5.0f, 0.0f, 0.0f));
    setupPanelMaterials(rightPanelObj, glm::vec3(0.0f, 5.0f, 0.0f));
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

        if (clickedData->slidingChannel) {
            clickedData->slidingChannel->stop();
            clickedData->slidingChannel = nullptr;
        }
        if (audioSystem && soundSlideIn) {
            clickedData->slidingChannel = audioSystem->playSoundEx(soundSlideIn);
        }
    }
    else {
        clickedData->isActivated = true;

        if (clickedData->slidingChannel) {
            clickedData->slidingChannel->stop();
            clickedData->slidingChannel = nullptr;
        }
        if (audioSystem && soundSlideOut) {
            clickedData->slidingChannel = audioSystem->playSoundEx(soundSlideOut);
        }

        std::vector<bool> gridOccupied(rows * cols * cols, false);

        for (auto& c : coffins) {
            if (c.isActivated && !c.isBouncingBack && &c != clickedData) {
                int r = c.row;
                for (int step = 0; step < c.currentTargetLevel; ++step) {
                    int markX = (c.wall == WallSide::Left) ? c.col : step;
                    int markZ = (c.wall == WallSide::Right) ? c.col : step;

                    gridOccupied[r * (cols * cols) + markX * cols + markZ] = true;
                }
            }
        }

        int r = clickedData->row;
        int maxAllowedLevel = cols;

        for (int step = 0; step < cols; ++step) {
            int checkX = (clickedData->wall == WallSide::Left) ? clickedData->col : step;
            int checkZ = (clickedData->wall == WallSide::Right) ? clickedData->col : step;

            if (gridOccupied[r * (cols * cols) + checkX * cols + checkZ]) {
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
    float animSpeed = 25.0f;
    float fadeDistance = 4.0f;

    for (auto& coffin : coffins) {
        if (!coffin.gameObject || !coffin.transform) continue;

        float oldExtension = coffin.currentExtensionAnim;

        if (coffin.isActivated) {
            float targetDistance = 0.0f;
            if (coffin.currentTargetLevel > 0) {
                float t = static_cast<float>(coffin.currentTargetLevel - 1) / static_cast<float>(cols - 1);
                targetDistance = minExtensionDistance + t * (maxExtensionDistance - minExtensionDistance);
            }
            else {
                targetDistance = minExtensionDistance * 0.8f;
            }

            if (coffin.currentExtensionAnim < targetDistance) {
                coffin.currentExtensionAnim += animSpeed * deltaTime;

                if (coffin.slidingChannel) {
                    float distanceLeft = targetDistance - coffin.currentExtensionAnim;
                    float volume = 1.0f;
                    if (distanceLeft < fadeDistance) {
                        volume = std::max(0.0f, distanceLeft / fadeDistance);
                    }
                    coffin.slidingChannel->setVolume(volume);
                }

                if (coffin.currentExtensionAnim >= targetDistance) {
                    coffin.currentExtensionAnim = targetDistance;

                    if (coffin.slidingChannel) {
                        coffin.slidingChannel->stop();
                        coffin.slidingChannel = nullptr;
                    }

                    if (coffin.isBouncingBack) {
                        if (audioSystem && soundCollide) {
                            audioSystem->playSoundEx(soundCollide);
                        }

                        coffin.isActivated = false;
                        coffin.isBouncingBack = false;
                        coffin.currentTargetLevel = 0;

                        if (audioSystem && soundSlideIn) {
                            coffin.slidingChannel = audioSystem->playSoundEx(soundSlideIn);
                        }
                    }
                }
            }
        }
        else {
            if (coffin.currentExtensionAnim > 0.0f) {
                coffin.currentExtensionAnim -= animSpeed * deltaTime;

                if (coffin.slidingChannel) {
                    float volume = 1.0f;
                    if (coffin.currentExtensionAnim < fadeDistance) {
                        volume = std::max(0.0f, coffin.currentExtensionAnim / fadeDistance);
                    }
                    coffin.slidingChannel->setVolume(volume);
                }

                if (coffin.currentExtensionAnim <= 0.0f) {
                    coffin.currentExtensionAnim = 0.0f;

                    if (coffin.slidingChannel) {
                        coffin.slidingChannel->stop();
                        coffin.slidingChannel = nullptr;
                    }
                    if (audioSystem && soundClose) {
                        audioSystem->playSoundEx(soundClose);
                    }
                }
            }
        }

        if (coffin.currentExtensionAnim != oldExtension) {
            glm::vec3 animOffset = glm::vec3(0.0f);
            if (coffin.wall == WallSide::Left) {
                animOffset = glm::vec3(0.0f, 0.0f, coffin.currentExtensionAnim * w1_extendDirZ);
            }
            else {
                animOffset = glm::vec3(coffin.currentExtensionAnim * w2_extendDirX, 0.0f, 0.0f);
            }
            coffin.transform->position = coffin.basePosition + animOffset;
            coffin.transform->isDirty = true;
        }
    }

    auto updatePanelColors = [&](GameObject* panel, WallSide side, glm::vec3 activeColor) {
        if (!panel) return;

        static std::vector<std::vector<bool>> grid(rows, std::vector<bool>(cols, false));
        for (int r = 0; r < rows; ++r) std::fill(grid[r].begin(), grid[r].end(), false);

        for (auto& coffin : coffins) {
            if (coffin.wall == side && coffin.isActivated && !coffin.isBouncingBack) {
                int mappedRow = (rows - 1) - coffin.row;
                int mappedCol = (side == WallSide::Right) ? coffin.col : ((cols - 1) - coffin.col);

                grid[mappedRow][mappedCol] = true;
            }
        }

        static std::vector<std::vector<bool>> lastGridLeft(rows, std::vector<bool>(cols, false));
        static std::vector<std::vector<bool>> lastGridRight(rows, std::vector<bool>(cols, false));
        auto& lastGrid = (side == WallSide::Left) ? lastGridLeft : lastGridRight;

        bool gridChanged = false;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (grid[r][c] != lastGrid[r][c]) {
                    gridChanged = true;
                    break;
                }
            }
            if (gridChanged) break;
        }

        if (!gridChanged) return;

        lastGrid = grid;

        panel->TraverseChildren([&](GameObject* child) {
            auto* render = child->GetComponent<RenderComponent>();
            if (!render) return;

            std::string objName = child->name;
            std::string parentName = child->GetParent() ? child->GetParent()->name : "";

            bool isCable = (objName.find("->") != std::string::npos) || (parentName.find("->") != std::string::npos);
            bool isEndObject = (objName.find("End") != std::string::npos) || (parentName.find("End") != std::string::npos);

            bool isPoint = false;
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    std::string cubeName = std::to_string(r + 1) + "x" + std::to_string(c + 1);
                    if (objName.find(cubeName) != std::string::npos || parentName.find(cubeName) != std::string::npos) {
                        isPoint = true;
                        break;
                    }
                }
                if (isPoint) break;
            }

            if (!isPoint && !isEndObject && !isCable) return;

            bool shouldBeActive = false;

            if (isEndObject) {
                shouldBeActive = true;
            }
            else if (isCable) {
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
            else if (isPoint) {
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        std::string cubeName = std::to_string(r + 1) + "x" + std::to_string(c + 1);
                        if (objName.find(cubeName) != std::string::npos || parentName.find(cubeName) != std::string::npos) {
                            if (grid[r][c]) shouldBeActive = true;
                        }
                    }
                }
            }

            bool wasChanged = false;
            for (auto& mesh : render->meshes) {
                std::shared_ptr<Material> targetMat = shouldBeActive ? activeMaterials[child] : inactiveMaterials[child];

                if (targetMat && mesh.material != targetMat) {
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

    static std::vector<std::vector<bool>> gridLeft(rows, std::vector<bool>(cols, false));
    static std::vector<std::vector<bool>> gridRight(rows, std::vector<bool>(cols, false));

    for (int r = 0; r < rows; ++r) {
        std::fill(gridLeft[r].begin(), gridLeft[r].end(), false);
        std::fill(gridRight[r].begin(), gridRight[r].end(), false);
    }

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

    static std::vector<std::vector<bool>> lastGridLeftDFS(rows, std::vector<bool>(cols, false));
    static std::vector<std::vector<bool>> lastGridRightDFS(rows, std::vector<bool>(cols, false));

    bool dfsNeeded = false;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (gridLeft[r][c] != lastGridLeftDFS[r][c] || gridRight[r][c] != lastGridRightDFS[r][c]) {
                dfsNeeded = true;
                break;
            }
        }
        if (dfsNeeded) break;
    }

    if (dfsNeeded) {
        lastGridLeftDFS = gridLeft;
        lastGridRightDFS = gridRight;

        auto hasPath = [&](const std::vector<std::vector<bool>>& grid, std::pair<int, int> start, std::pair<int, int> end) -> bool {
            if (!grid[start.first][start.second] || !grid[end.first][end.second]) return false;

            static std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
            for (int r = 0; r < rows; ++r) {
                std::fill(visited[r].begin(), visited[r].end(), false);
            }

            static std::vector<std::pair<int, int>> stack;
            stack.clear();

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
            //spdlog::info("Lewa Sciana Zostala Polaczona");
            isLeftSolved = true;
        }
        else if (!currentLeftSolved && isLeftSolved) {
            isLeftSolved = false;
        }

        if (currentRightSolved && !isRightSolved) {
            //spdlog::info("Prawa Sciana Zostala Polaczona");
            isRightSolved = true;
        }
        else if (!currentRightSolved && isRightSolved) {
            isRightSolved = false;
        }

        if (isLeftSolved && isRightSolved && !isPuzzleSolved) {
            //spdlog::warn("ZAGADKA KREMATORIUM ZOSTALA ROZWIAZANA W PELNI WOOOOOOOOOOOW");
            isPuzzleSolved = true;

            for (auto& coffin : coffins) {
                if (coffin.isActivated) {
                    coffin.isActivated = false;
                    coffin.isBouncingBack = false;
                    coffin.currentTargetLevel = 0;

                    if (coffin.slidingChannel) {
                        coffin.slidingChannel->stop();
                        coffin.slidingChannel = nullptr;
                    }
                    if (audioSystem && soundSlideIn) {
                        coffin.slidingChannel = audioSystem->playSoundEx(soundSlideIn);
                    }
                }
            }
        }
    }
}

void CrematoriumPuzzle::SetupAudio(AudioSystem* audioSys, FMOD::Sound* slideOut, FMOD::Sound* slideIn, FMOD::Sound* collide, FMOD::Sound* close, FMOD::Sound* puzzleSolved) {
    audioSystem = audioSys;
    soundSlideOut = slideOut;
    soundSlideIn = slideIn;
    soundCollide = collide;
    soundClose = close;
    soundPuzzleSolved = puzzleSolved;
}
void CrematoriumPuzzle::Reset()
{
    for (auto& coffin : coffins)
    {
        if (coffin.slidingChannel) {
            coffin.slidingChannel->stop();
            coffin.slidingChannel = nullptr;
        }

        coffin.isActivated       = false;
        coffin.isBouncingBack    = false;
        coffin.currentTargetLevel = 0;
        coffin.currentExtensionAnim = 0.0f;

        if (coffin.transform) {
            coffin.transform->position = coffin.basePosition;
            coffin.transform->isDirty  = true;
        }
    }

    isLeftSolved  = false;
    isRightSolved = false;
    isPuzzleSolved = false;

    auto resetPanel = [&](GameObject* panel) {
        if (!panel) return;
        panel->TraverseChildren([&](GameObject* child) {
            auto* render = child->GetComponent<RenderComponent>();
            if (!render) return;
            if (inactiveMaterials.count(child)) {
                for (auto& mesh : render->meshes) {
                    std::string objName = child->name;
                    std::string parentName = child->GetParent() ? child->GetParent()->name : "";
                    bool isEndObject = (objName.find("End") != std::string::npos) || (parentName.find("End") != std::string::npos);
                    if (!isEndObject && activeMaterials.count(child))
                        mesh.material = inactiveMaterials[child];
                }
                child->NotifyChanged();
            }
        });
    };
    resetPanel(leftPanelObj);
    resetPanel(rightPanelObj);
}