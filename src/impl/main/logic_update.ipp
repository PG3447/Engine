struct PuzzleSlot {
    glm::vec3 targetRotation;
    GameObject* occupant = nullptr;
    GameObject* slotObject = nullptr;
    GameObject* expectedObject = nullptr;
    GameObject* lightObject = nullptr;
};

GameObject* puzzleRewardObject = nullptr;

std::unordered_map<GameObject*, PuzzleSlot> machineSlotsMap;
bool isMachineFixed = false;

std::unordered_map<GameObject*, PuzzleSlot> puzzleSlotsMap; // klucz = slotObject
std::unordered_map<GameObject*, glm::vec3>  objectOriginalRotations;
std::unordered_map<GameObject*, glm::vec3> objectOriginalPositions;
std::unordered_map<GameObject*, glm::vec3>  objectOriginalColliderSizes;

glm::vec3 gearHeldOffset = glm::vec3(1.25f, -1.2f, -5.55f);
glm::vec3 gearHeldRotation = glm::vec3(0.0f, 234.0f, 100.0f);

GameObject* machineStartButton = nullptr;
std::unordered_map<std::string, GameObject*> machineLights;
GameObject* fixedGear1 = nullptr;
GameObject* fixedGear2 = nullptr;

static bool IsPlayerHierarchy(GameObject* go) {
    if (!go) return false;
    for (GameObject* node = go; node; node = node->GetParent()) {
        if (node->name == "Gracz1" || node->name == "Gracz2") return true;
    }
    return false;
}

static RaycastHit FindInteractionHit(RaycastComponent* playerRaycast, GameObject* excludeObject = nullptr) {
    RaycastHit best;
    best.distance = 1e30f;
    for (const auto& hit : playerRaycast->raycastHits) {
        if (!hit.hit || !hit.hitObject) continue;
        if (IsPlayerHierarchy(hit.hitObject)) continue;
        if (excludeObject && hit.hitObject == excludeObject) continue;
        if (hit.distance < best.distance) best = hit;
    }
    return (best.distance < 1e30f) ? best : RaycastHit{};
}

static void SaveColliderSizeIfNeeded(GameObject* obj) {
    if (!obj) return;
    auto* col = obj->GetComponent<ColliderComponent>();
    if (!col) return;
    if (!objectOriginalColliderSizes.count(obj))
        objectOriginalColliderSizes[obj] = col->halfSize;
}

static void DisableHeldCollider(GameObject* obj) {
    SaveColliderSizeIfNeeded(obj);
    if (auto* col = obj->GetComponent<ColliderComponent>())
        col->halfSize = glm::vec3(0.0f);
}

static void RestoreColliderSize(GameObject* obj) {
    if (!obj) return;
    auto* col = obj->GetComponent<ColliderComponent>();
    if (!col) return;
    if (objectOriginalColliderSizes.count(obj))
        col->halfSize = objectOriginalColliderSizes[obj];
}

GameObject* SpawnGearReward(Scene* scene, const glm::vec3& position, const std::string& name) {
    if (!gearModel) {
        spdlog::error("gearModel nie jest zaladowany!");
        return nullptr;
    }

    GameObject* gear = gearModel->Instantiate(*scene, nullptr, nullptr);
    gear->name = name;

    TransformComponent* tr = gear->GetComponent<TransformComponent>();
    tr->position = position;
    tr->scale = glm::vec3(2.0f);
    tr->isDirty = true;

    RigidbodyComponent* rb = gear->AddComponent<RigidbodyComponent>();
    rb->useGravity = true;
    rb->isStatic = false;
    rb->mass = 2.0f;

    rb->physicsPosition = position;
    rb->previousPosition = position;

    ColliderComponent* col = gear->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(1.0f, 0.2f, 1.0f);

    pickupObjects.insert(gear);
    objectOriginalRotations[gear] = glm::vec3(0.0f);

    return gear;
}

void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

std::string LoadLoreFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("Nie mozna otworzyc pliku z notatka: {}", filepath);
        return "Brak pliku " + filepath;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    ReplaceAll(content, "\xE2\x96\x8C", std::string(1, (char)127));

    std::istringstream contentStream(content);
    std::string line;
    std::string result = "";

    bool isHandwriting = false;

    while (std::getline(contentStream, line)) {
        if (line.empty() || line.find_first_not_of("\r") == std::string::npos) {
            result += "\n";
            continue;
        }

        std::istringstream words(line);
        std::string word;
        size_t currentLineLength = 0;
        bool firstWord = true;

        while (words >> word) {
            if (word.find('^') != std::string::npos) {
                isHandwriting = !isHandwriting;
            }

            size_t currentMaxChars = isHandwriting ? 45 : 65;

            size_t wordLen = word.length();
            if (word.find('^') != std::string::npos) wordLen--;

            if (!firstWord && currentLineLength + wordLen + 1 > currentMaxChars) {
                result += "\n";
                currentLineLength = 0;
                firstWord = true;
            }
            if (!firstWord) {
                result += " ";
                currentLineLength++;
            }
            result += word;
            currentLineLength += wordLen;
            firstWord = false;
        }
        result += "\n";
    }
    return result;
}

GameObject* SpawnLoreNote(Scene* scene, Prefab* paperPrefab, const glm::vec3& position, const std::string& filepath, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f), GameObject* parent = nullptr) {
    if (!paperPrefab) {
        spdlog::error("Prefab kartki nie zostal zaladowany!");
        return nullptr;
    }

    GameObject* note = paperPrefab->Instantiate(*scene, parent, nullptr);
    note->name = "LoreNote";

    TransformComponent* tr = note->GetComponent<TransformComponent>();
    tr->position = position;
    tr->rotation = rotation;
	tr->scale = scale;
    tr->isDirty = true;

    ColliderComponent* col = note->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(1.0f, 0.5f, 1.0f);

    noteContents[note] = LoadLoreFromFile(filepath);

    return note;
}

void OnPuzzleSolved(Scene* scene, AudioSystem* audioSystem = nullptr, FMOD::Sound* sndGear = nullptr) {
    spdlog::info("Puzzle rozwiazany!");

    if (puzzleRewardObject != nullptr) return;

    puzzleRewardObject = SpawnGearReward(scene, glm::vec3(-73.721, 8.0f, -200.833), "Gear_Rentgen");

    if (audioSystem && sndGear) {
        audioSystem->playSound(sndGear);
    }
}

bool IsPuzzleSolved() {
    if (puzzleSlotsMap.empty()) return false;
    for (auto& [slotGO, slot] : puzzleSlotsMap) {
        if (slot.occupant == nullptr) return false;             // slot pusty
        if (slot.occupant != slot.expectedObject) return false; // zla kostka
    }
    return true;
}

void HandlePlayerInteraction(
    ECS& ecs,
    const std::string& inputAction,
    RaycastComponent* playerRaycast,
    GameObject* playerCamera,
    GameObject*& myHeldObject,
    GameObject* otherPlayerHeldObject,
    Scene* scene,
    std::unordered_map<GameObject*, float>& rotatingObjects,
    std::unordered_set<GameObject*>& rotatingInProgress,
    float& outShakeTimer,
    AnimatorComponent* playerAnimator,
    Prefab* playerPrefab,
    AudioSystem* audioSystem = nullptr,
    FMOD::Sound* soundPaper = nullptr,
    FMOD::Sound* soundDoorOpen = nullptr,
    FMOD::Sound* soundDoorCloseStart = nullptr,
    FMOD::Sound* soundBtnClick = nullptr,
    FMOD::Sound* soundUnlock = nullptr,
    FMOD::Sound* soundPickup = nullptr,
    FMOD::Sound* soundInsert = nullptr,
    FMOD::Sound* soundDoorLocked = nullptr,
    FMOD::Sound* soundPuzzleSolved = nullptr
) {
    if (!ecs.GetSystem<HID>()->is_action_just_pressed(inputAction)) return;
    // upuszczanie / wkladanie
    if (myHeldObject != nullptr) {
        TransformComponent* camTr = playerCamera->GetComponent<TransformComponent>();
        TransformComponent* heldTr = myHeldObject->GetComponent<TransformComponent>();
        CameraComponent* camComp = playerCamera->GetComponent<CameraComponent>();

        PuzzleSlot* targetSlot = nullptr;
        bool isMachineSlot = false;
        float bestSlotDistance = 1e30f;

        for (const auto& hit : playerRaycast->raycastHits) {
            if (!hit.hit || !hit.hitObject || IsPlayerHierarchy(hit.hitObject)) continue;

            if (puzzleSlotsMap.count(hit.hitObject)) {
                PuzzleSlot& slot = puzzleSlotsMap[hit.hitObject];
                if (slot.occupant == nullptr && hit.distance < bestSlotDistance) {
                    bestSlotDistance = hit.distance;
                    targetSlot = &slot;
                    isMachineSlot = false;
                }
            }
            else if (machineSlotsMap.count(hit.hitObject)) {
                PuzzleSlot& slot = machineSlotsMap[hit.hitObject];
                if (slot.occupant == nullptr && myHeldObject->name.find("Gear") != std::string::npos && hit.distance < bestSlotDistance) {
                    bestSlotDistance = hit.distance;
                    targetSlot = &slot;
                    isMachineSlot = true;
                }
            }
        }


        if (targetSlot != nullptr) {
            myHeldObject->SetParent(targetSlot->slotObject);
        }
        else {
            myHeldObject->SetParent(scene->GetRoot());
        }

        if (targetSlot != nullptr) {
            TransformComponent* slotTr = targetSlot->slotObject->GetComponent<TransformComponent>();
            TransformHelper::computeModelMatrix(*slotTr);

            myHeldObject->SetParent(targetSlot->slotObject);

            TransformComponent* heldTr = myHeldObject->GetComponent<TransformComponent>();
            heldTr->parent = slotTr;
            if (isMachineSlot) {
                heldTr->position = glm::vec3(0.0f);
                heldTr->rotation = glm::vec3(0.0f, 0.0f, 90.0f);
                heldTr->scale    = glm::vec3(0.4375f);
            } else {
                heldTr->position = glm::vec3(0.0f);
                heldTr->rotation = targetSlot->targetRotation;
            }

            TransformHelper::computeModelMatrix(slotTr->modelMatrix, *heldTr);

            if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                glm::vec3 globalPos = TransformHelper::getGlobalPosition(*heldTr);
                rb->physicsPosition = globalPos;
                rb->previousPosition = globalPos;
            }

            targetSlot->occupant = myHeldObject;

            if (audioSystem && soundInsert) {
                audioSystem->playSound(soundInsert);
            }

            if (isMachineSlot) {
                std::string slotName = targetSlot->slotObject->name;
                std::string lightName = "";

                if (slotName == "MachineSlot_1") lightName = "lights_1";
                else if (slotName == "MachineSlot_2") lightName = "lights_3";
                else if (slotName == "MachineSlot_3") lightName = "lights_4";

                if (!lightName.empty() && machineLights.count(lightName)) {
                    if (auto lc = machineLights[lightName]->GetComponent<LightComponent>()) {
                        lc->isOn = true;
                    }
                }
            }
            else {
                if (IsPuzzleSolved()) {
                    static bool rentgenPuzzleSolvedPlayed = false;
                    if (!rentgenPuzzleSolvedPlayed && audioSystem && soundPuzzleSolved) {
                        audioSystem->playSound(soundPuzzleSolved);
                        rentgenPuzzleSolvedPlayed = true;
                    }

                    OnPuzzleSolved(scene, audioSystem, soundPuzzleSolved);
                }
            }

            if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                rb->useGravity = false;
                rb->isStatic = true;
                rb->velocity = glm::vec3(0.0f);
                rb->acceleration = glm::vec3(0.0f);
            }
            if (auto col = myHeldObject->GetComponent<ColliderComponent>()) {
                col->halfSize = glm::vec3(0.0f);
            }
        }
        else {
            glm::vec3 dropPos = TransformHelper::getGlobalPosition(*camTr) + (camComp->state.Front * 5.0f);
            TransformHelper::setGlobalPosition(*heldTr, dropPos, nullptr);
            heldTr->rotation = objectOriginalRotations.count(myHeldObject) ? objectOriginalRotations[myHeldObject] : glm::vec3(0.0f);

            if (myHeldObject->name.find("Gear") != std::string::npos) {
                heldTr->scale = glm::vec3(2.0f);
            }

            RestoreColliderSize(myHeldObject);

            heldTr->isDirty = true;

            if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                TransformHelper::computeModelMatrix(*heldTr);
                glm::vec3 globalPos = TransformHelper::getGlobalPosition(*heldTr);
                rb->useGravity = true;
                rb->isStatic = false;
                rb->previousPosition = globalPos;
                rb->physicsPosition = globalPos;
                rb->velocity = glm::vec3(0.0f);
                rb->acceleration = glm::vec3(0.0f);
            }
        }

        PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::DROP_ANIM_INDEX);

        myHeldObject = nullptr;
    }
    else {
        RaycastHit hit = FindInteractionHit(playerRaycast, otherPlayerHeldObject);
        if (hit.hitObject != nullptr) {
            // Obracanie
            if (rotatableObjects.count(hit.hitObject)) {
                if (!rotatingInProgress.count(hit.hitObject)) {
                    rotatingObjects[hit.hitObject] = 60.0f;
                    rotatingInProgress.insert(hit.hitObject);
                    PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);

                    if (audioSystem && soundPaper) {
                        audioSystem->playSound(soundPaper);
                    }
                }
                else if (hit.hitObject == machineStartButton) {
                    bool allInserted = true;
                    for (auto& [mSlotGO, mSlot] : machineSlotsMap) {
                        if (mSlot.occupant == nullptr) {
                            allInserted = false;
                            break;
                        }
                    }

                    if (allInserted && !isMachineFixed) {
                        isMachineFixed = true; // Maszyna rusza!
                        outShakeTimer = SHAKE_DURATION; // Mocne trzesienie kamery z impaktem

                        spdlog::info("Guzik START wcisniety! Maszyna ruszyla, otwieranie drzwi.");

                        if (audioSystem && soundUnlock) audioSystem->playSound(soundUnlock);

                        // Otwieramy drzwi
                        for (GameObject* hinge : mainRoomDoors) {
                            if (hinge && hinge->name == "Hinge_DrzwiDoRentgen") {
                                DoorState& dState = toiletDoorsMap[hinge];
                                dState.isOpen = true;
                                dState.targetAngle = dState.openAngle;
                                if (auto col = hinge->GetComponent<ColliderComponent>()) {
                                    col->halfSize = glm::vec3(0.0f);
                                }
                            }
                        }
                    }
                    else if (!isMachineFixed) {
                        // Brak zebatek - blad
                        outShakeTimer = SHAKE_DURATION * 0.5f; // Ma�e trz�sienie
                        if (audioSystem && soundDoorLocked) audioSystem->playSound(soundDoorLocked);
                    }
                }
            }
            // PAIN
            if (majorDoors.count(hit.hitObject)) {
                if (can_open_door_1) {
                    for (GameObject* door : majorDoors) {
                        TransformComponent* t = door->GetComponent<TransformComponent>();
                        if (t) {
                            t->position = glm::vec3(-1000.0f, -1000.0f, -1000.0f);
                            t->isDirty = true;
                        }
                    }
                }
                else {
                    outShakeTimer = SHAKE_DURATION;
                    if (audioSystem && soundDoorLocked) {
                        audioSystem->playSound(soundDoorLocked);
                    }
                }
                PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);
            }
            // Otwieranie drzwi
            else if (toiletDoorsMap.count(hit.hitObject)) {
                DoorState& state = toiletDoorsMap[hit.hitObject];
                if (state.requiresUnlock && !can_open_door_1) {
                    outShakeTimer = SHAKE_DURATION;
                    if (audioSystem && soundDoorLocked) audioSystem->playSound(soundDoorLocked);
                }
                else if (state.canBeClicked) {
                    state.isOpen = !state.isOpen;
                    state.targetAngle = state.isOpen ? state.openAngle : state.closedAngle;
                    if (auto col = hit.hitObject->GetComponent<ColliderComponent>()) {
                        col->halfSize = state.isOpen ? state.openHalfSize : state.closedHalfSize;
                        col->offset = state.isOpen ? state.openOffset : state.originalOffset;
                    }
                    PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);

                    if (state.isOpen) {
                        if (audioSystem && soundDoorOpen) {
                            audioSystem->playSound(soundDoorOpen);
                        }
                    }
                    else {
                        if (audioSystem && soundDoorCloseStart) {
                            audioSystem->playSound(soundDoorCloseStart);
                        }
                    }
                }
            }
            // Otwieranie szafki
            else if (cabinetsMap.count(hit.hitObject)) {
                if (!isCabinetButtonPushed) {
                    isCabinetButtonPushed = true;
                    CabinetState& state = cabinetsMap[hit.hitObject];
                    state.isOpen = true;
                    state.targetAngle = 120.0f;

                    if (audioSystem) {
                        if (soundBtnClick) audioSystem->playSound(soundBtnClick);
                        if (soundUnlock)   audioSystem->playSound(soundUnlock);
                    }

                    for (auto& [slotGO, mSlot] : machineSlotsMap) {
                        if (auto col = slotGO->GetComponent<ColliderComponent>()) {
                            col->halfSize = glm::vec3(1.5f, 1.5f, 1.5f);

                            if (auto tr = slotGO->GetComponent<TransformComponent>()) {
                                tr->isDirty = true;
                            }
                        }
                    }
                    if (machineStartButton) {
                        if (auto col = machineStartButton->GetComponent<ColliderComponent>()) {
                            col->halfSize = glm::vec3(1.0f, 1.0f, 1.0f);
                            if (auto tr = machineStartButton->GetComponent<TransformComponent>()) { tr->isDirty = true; }
                        }
                    }

                    for (GameObject* hinge : mainRoomDoors) {
                        if (hinge && hinge->name != "Hinge_DrzwiDoRentgen" && toiletDoorsMap.count(hinge)) {
                            DoorState& dState = toiletDoorsMap[hinge];
                            dState.isOpen = true;
                            dState.targetAngle = dState.openAngle;

                            if (auto col = hinge->GetComponent<ColliderComponent>()) {
                                col->halfSize = glm::vec3(0.0f);
                            }
                        }
                    }
                    PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);
                }
            }
            if (puzzleSlotsMap.count(hit.hitObject)) {
                PuzzleSlot& slot = puzzleSlotsMap[hit.hitObject];
                if (slot.occupant != nullptr && slot.occupant != otherPlayerHeldObject) {
                    myHeldObject = slot.occupant;
                    slot.occupant = nullptr;

                    myHeldObject->SetParent(playerCamera);

                    TransformComponent* heldTr = myHeldObject->GetComponent<TransformComponent>();
                    heldTr->position = glm::vec3(1.0f, -1.0f, -3.0f);

                    if (myHeldObject->name.find("Gear") != std::string::npos) {
                        heldTr->position = gearHeldOffset;
                        heldTr->rotation = gearHeldRotation;
                        heldTr->scale = glm::vec3(2.0f);
                    }
                    else {
                        heldTr->position = glm::vec3(1.0f, -1.0f, -3.0f);
                        heldTr->rotation = glm::vec3(0.0f);
                    }
                    heldTr->isDirty = true;

                    if (auto col = myHeldObject->GetComponent<ColliderComponent>()) {
                        DisableHeldCollider(myHeldObject);
                    }

                    if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                        rb->useGravity = false;
                        rb->isStatic = true;
                    }

                    if (audioSystem && soundPickup) audioSystem->playSound(soundPickup);
                    PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::PICKUP_ANIM_INDEX);
                }
            }
            else if (pickupObjects.count(hit.hitObject) && hit.hitObject != otherPlayerHeldObject) {
                myHeldObject = hit.hitObject;

                for (auto& [slotGO, slot] : puzzleSlotsMap) {
                    if (slot.occupant == myHeldObject) {
                        puzzleSlotsMap[slotGO].occupant = nullptr;
                        break;
                    }
                }

                myHeldObject->SetParent(playerCamera);

                TransformComponent* heldTr = myHeldObject->GetComponent<TransformComponent>();
                heldTr->position = glm::vec3(1.0f, -1.0f, -3.0f);

                if (myHeldObject->name.find("Gear") != std::string::npos) {
                    heldTr->rotation = glm::vec3(45.0f, 30.0f, 0.0f);
                }
                else {
                    heldTr->rotation = glm::vec3(0.0f);
                }
                heldTr->isDirty = true;

                if (auto col = myHeldObject->GetComponent<ColliderComponent>()) {
                    DisableHeldCollider(myHeldObject);
                }

                if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                    rb->useGravity = false;
                    rb->isStatic = true;
                }

                if (audioSystem && soundPickup) audioSystem->playSound(soundPickup);
                PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::PICKUP_ANIM_INDEX);
            }
            // guzik szafki z zebatkami
            else if (hit.hitObject == machineStartButton) {
                bool allInserted = true;
                for (auto& [mSlotGO, mSlot] : machineSlotsMap) {
                    if (mSlot.occupant == nullptr) {
                        allInserted = false;
                        break;
                    }
                }

                if (allInserted && !isMachineFixed) {
                    isMachineFixed = true;
                    outShakeTimer = SHAKE_DURATION;

                    spdlog::info("Maszyna ruszyla - otwieranie drzwi.");

                    if (audioSystem && soundUnlock) audioSystem->playSound(soundUnlock);

                    for (GameObject* hinge : mainRoomDoors) {
                        if (hinge && hinge->name == "Hinge_DrzwiDoRentgen") {
                            DoorState& dState = toiletDoorsMap[hinge];
                            dState.isOpen = true;
                            dState.targetAngle = dState.openAngle;
                            if (auto col = hinge->GetComponent<ColliderComponent>()) {
                                col->halfSize = glm::vec3(0.0f);
                            }
                        }
                    }
                }
                else if (!isMachineFixed) {
                    outShakeTimer = SHAKE_DURATION * 0.5f;
                    if (audioSystem && soundDoorLocked) audioSystem->playSound(soundDoorLocked);
                }
            }
            // zagadka z trumnami
            else if (hit.hitObject->name.find("Coffin") != std::string::npos) {
                crematoriumPuzzle.ToggleCoffin(hit.hitObject);
                PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);
            }
        }
    }
}

void HandleAltRotate(
    ECS& ecs,
    const std::string& inputAction,
    RaycastComponent* playerRaycast,
    std::unordered_map<GameObject*, float>& rotatingObjects,
    std::unordered_set<GameObject*>& rotatingInProgress,
    AudioSystem* audioSystem = nullptr,
    FMOD::Sound* soundPaper = nullptr
) {
    if (!ecs.GetSystem<HID>()->is_action_just_pressed(inputAction)) return;
    RaycastHit hit = FindInteractionHit(playerRaycast);
    if (hit.hitObject && rotatableObjects.count(hit.hitObject)) {
        if (!rotatingInProgress.count(hit.hitObject)) {
            rotatingObjects[hit.hitObject] = -60.0f;
            rotatingInProgress.insert(hit.hitObject);

            if (audioSystem && soundPaper) {
                audioSystem->playSound(soundPaper);
            }
        }
    }
}

void UpdateDoors(float deltaTime, AudioSystem* audioSystem = nullptr, FMOD::Sound* soundClosed = nullptr) {
    float doorAnimSpeed = 180.0f;
    for (auto& [doorObj, state] : toiletDoorsMap) {
        if (std::abs(state.currentAngle - state.targetAngle) > 0.1f) {
            float direction = (state.targetAngle > state.currentAngle) ? 1.0f : -1.0f;
            state.currentAngle += direction * doorAnimSpeed * deltaTime;

            bool justFinished = false;
            if ((direction > 0.0f && state.currentAngle > state.targetAngle) ||
                (direction < 0.0f && state.currentAngle < state.targetAngle)) {
                state.currentAngle = state.targetAngle;
                justFinished = true;
            }

            TransformComponent* hingeTr = state.hinge->GetComponent<TransformComponent>();
            if (hingeTr) {
                hingeTr->rotation.y = state.currentAngle;
                hingeTr->isDirty = true;
            }

            if (justFinished && !state.isOpen) {
                if (audioSystem && soundClosed) {
                    audioSystem->playSound(soundClosed);
                }
            }
        }
    }
}

void UpdateCabinets(float deltaTime) {
    float animSpeed = 180.0f;
    float buttonSpeed = 5.0f;

    for (auto& [buttonObj, state] : cabinetsMap) {
        if (std::abs(state.currentAngle - state.targetAngle) > 0.1f) {
            float direction = (state.targetAngle > state.currentAngle) ? 1.0f : -1.0f;
            state.currentAngle += direction * animSpeed * deltaTime;

            if ((direction > 0.0f && state.currentAngle > state.targetAngle) ||
                (direction < 0.0f && state.currentAngle < state.targetAngle)) {
                state.currentAngle = state.targetAngle;
            }

            if (state.leftDoor) {
                TransformComponent* t = state.leftDoor->GetComponent<TransformComponent>();
                if (t) { t->rotation.y = -state.currentAngle; t->isDirty = true; }
            }
            if (state.rightDoor) {
                TransformComponent* t = state.rightDoor->GetComponent<TransformComponent>();
                if (t) { t->rotation.y = state.currentAngle; t->isDirty = true; }
            }
        }

        if (isCabinetButtonPushed && state.button) {
            TransformComponent* btnTr = state.button->GetComponent<TransformComponent>();
            btnTr->position = glm::mix(btnTr->position, state.buttonTargetPos, deltaTime * buttonSpeed);
            if (glm::distance(btnTr->position, state.buttonTargetPos) > 0.01f) {
                btnTr->isDirty = true;
            }
        }
    }
}
GameObject* CreateInteractableDoor(Scene* scene, Prefab* prefab, Shader* shader,
    const std::string& name,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& pivotOffset,
    const glm::vec3& colliderHalfSize,
    float openAngle,
    float baseRotationY = 90.0f,
    std::optional<glm::vec3> openColliderHalfSize = std::nullopt,
    std::optional<glm::vec3> openColliderOffset = std::nullopt,
    std::optional<glm::vec3> closedOffsetAdjust = std::nullopt)
{
    GameObject* hinge = scene->CreateGameObject(nullptr);
    hinge->name = "Hinge_" + name;
    TransformComponent* hingeTr = hinge->AddComponent<TransformComponent>();
    hingeTr->position = position + pivotOffset;

    GameObject* door = prefab->Instantiate(*scene, hinge, shader);
    door->name = name;
    TransformComponent* doorTr = door->GetComponent<TransformComponent>();
    doorTr->scale = scale;
    doorTr->rotation = glm::vec3(0.0f, baseRotationY, 0.0f);
    doorTr->position = -pivotOffset;

    glm::vec3 closedOffset = -pivotOffset + closedOffsetAdjust.value_or(glm::vec3(0.0f));

    ColliderComponent* col = hinge->AddComponent<ColliderComponent>();
    col->halfSize = colliderHalfSize;
    col->offset = closedOffset;
    col->isWalkable = false;
    col->affectsNavMesh = true;

    DoorState state;
    state.hinge = hinge;
    state.openAngle = openAngle;
    state.closedAngle = 0.0f;
    state.currentAngle = 0.0f;
    state.targetAngle = 0.0f;
    state.originalOffset = closedOffset;
    state.closedHalfSize = colliderHalfSize;
    state.openHalfSize = openColliderHalfSize.value_or(glm::vec3(1.0f, 10.0f, 1.0f));
    state.openOffset = openColliderOffset.value_or(glm::vec3(0.0f));
    toiletDoorsMap[hinge] = state;
    return hinge;
}

GameObject* CreateStaticObject(
    Scene* scene,
    Prefab* prefab,
    Shader* shader,
    const std::string& name,
    const glm::vec3& position,
    const glm::vec3& scale,
    std::optional<glm::vec3> rotation = std::nullopt,
    std::optional<glm::vec3> colliderHalfSize = std::nullopt,
    bool affectsNavMesh = false
) {
    GameObject* go = prefab->Instantiate(*scene, nullptr, shader);
    go->name = name;

    TransformComponent* tr = go->GetComponent<TransformComponent>();
    if (tr) {
        tr->position = position;
        tr->scale = scale;
        if (rotation.has_value())
            tr->rotation = rotation.value();
        tr->isDirty = true;
    }

    ColliderComponent* col = go->AddComponent<ColliderComponent>();
    if (colliderHalfSize.has_value())
        col->halfSize = colliderHalfSize.value();
    col->affectsNavMesh = affectsNavMesh;

    return go;
}

GameObject* CreateCockroachLeader(
    Scene& scene,
    Prefab& prefab,
    Shader* shader,
    const glm::vec3& homePos,
    float moveSpeed = 4.0f)
{
    GameObject* go = prefab.Instantiate(scene, nullptr, shader);
    go->name = "CockroachLeader";

    auto* tr = go->GetComponent<TransformComponent>();
    tr->position = homePos;
    tr->scale = glm::vec3(0.5f);
    tr->isDirty = true;

    /*
    auto* col = go->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(0.3f, 0.2f, 0.3f);
    */

    auto* nav = go->AddComponent<NavPathComponent>();
    nav->state = NavAgentState::ExternalControl;
    nav->moveSpeed = moveSpeed;
    nav->idleTimeMax = 0.0f;

    auto* leader = go->AddComponent<CockroachLeaderComponent>();
    leader->homePosition = homePos;
    leader->homeRadius = 15.0f;
    leader->homeTimeRequired = 8.0f;
    leader->exploreRadius = 50.0f;
    leader->exploreDuration = 20.0f;
    leader->detectionRadius = 25.0f;
    leader->escapeRadius = 35.0f;
    leader->idleWanderRadius = 8.0f;
    leader->state = LeaderState::Idle;

    return go;
}

GameObject* CreateCockroachFollower(
    Scene& scene,
    Prefab& prefab,
    Shader* shader,
    GameObject* leaderGO,
    const glm::vec3& spawnPos,
    float moveSpeed = 4.5f)
{
    GameObject* go = prefab.Instantiate(scene, nullptr, shader);
    go->name = "CockroachFollower";

    auto* tr = go->GetComponent<TransformComponent>();
    tr->position = spawnPos;
    tr->scale = glm::vec3(0.5f);
    tr->isDirty = true;


    /*
    auto* col = go->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(0.25f, 0.15f, 0.25f);
    */

    auto* nav = go->AddComponent<NavPathComponent>();
    nav->state = NavAgentState::ExternalControl;
    nav->moveSpeed = moveSpeed;
    nav->idleTimeMax = 0.0f;

    auto* follower = go->AddComponent<CockroachFollowerComponent>();
    follower->leaderGameObject = leaderGO;
    follower->followDistance = 6.0f;
    follower->followStopDistance = 2.0f;
    follower->idleWanderRadius = 6.0f;
    follower->state = FollowerState::Follow;

    return go;
}
void CheckFallenPickupObjects()
{
    for (auto* obj : pickupObjects)
    {
        if (obj == p1HeldObject || obj == p2HeldObject) continue;

        TransformComponent* tr = obj->GetComponent<TransformComponent>();
        if (tr == nullptr) continue;

        if (TransformHelper::getGlobalPosition(*tr).y < -1.0f)
        {
            auto posIt = objectOriginalPositions.find(obj);
            if (posIt == objectOriginalPositions.end()) continue;

            tr->position = posIt->second;
            tr->isDirty  = true;

            auto rotIt = objectOriginalRotations.find(obj);
            if (rotIt != objectOriginalRotations.end())
                tr->rotation = rotIt->second;

            for (auto& [slotGO, slot] : puzzleSlotsMap)
            {
                if (slot.occupant == obj)
                {
                    slot.occupant = nullptr;
                    break;
                }
            }

            if (RigidbodyComponent* rb = obj->GetComponent<RigidbodyComponent>())
                rb->velocity = glm::vec3(0.0f);

            spdlog::info("Obiekt '{}' spadl poza mape - reset na pozycje startowa", obj->name);
        }
    }
}