//void imgui_begin()
//{
//    ImGui_ImplOpenGL3_NewFrame();
//    ImGui_ImplGlfw_NewFrame();
//    ImGui::NewFrame();
//}
//
//GameObject* selectedGameObject = nullptr;
//static ImGuiTextFilter gameObjectFilter;
//
//bool GameObjectMatchesFilter(GameObject* obj, const ImGuiTextFilter& filter)
//{
//    if (!obj) return false;
//
//    const char* name = obj->name.empty() ? "GameObject" : obj->name.c_str();
//    if (filter.PassFilter(name))
//        return true;
//
//    for (GameObject* child : obj->GetChildren())
//    {
//        if (GameObjectMatchesFilter(child, filter))
//            return true;
//    }
//
//    return false;
//}
//
//void ShowGameObjectTree(Scene* activeScene, GameObject* obj, ImGuiTextFilter& filter)
//{
//    if (!obj) return;
//
//    bool filterActive = filter.IsActive();
//    if (filterActive && !GameObjectMatchesFilter(obj, filter))
//        return;
//
//    const char* displayName = obj->name.empty() ? "GameObject" : obj->name.c_str();
//
//    ImGuiTreeNodeFlags flags =
//        ImGuiTreeNodeFlags_OpenOnArrow |
//        ImGuiTreeNodeFlags_OpenOnDoubleClick |
//        ((obj == selectedGameObject) ? ImGuiTreeNodeFlags_Selected : 0);
//
//    if (!obj->HasChildren())
//        flags |= ImGuiTreeNodeFlags_Leaf;
//
//    if (filterActive)
//        flags |= ImGuiTreeNodeFlags_DefaultOpen;
//
//    bool opened = ImGui::TreeNodeEx((void*)obj, flags, "%s", displayName);
//
//    if (ImGui::IsItemClicked())
//    {
//        selectedGameObject = obj;
//        if (activeScene->GetECS().GetSystem<RaycastSystem>())
//            activeScene->GetECS().GetSystem<RaycastSystem>()->selectedObject = selectedGameObject;
//    }
//
//    if (opened) {
//        for (GameObject* child : obj->GetChildren())
//            ShowGameObjectTree(activeScene, child, filter);
//        ImGui::TreePop();
//    }
//}
//
//void ShowTransformEditor(GameObject* owner, TransformComponent& transform)
//{
//    glm::vec3 pos = TransformHelper::getLocalPosition(transform);
//    glm::vec3 rot = TransformHelper::getLocalRotation(transform);
//    glm::vec3 scale = TransformHelper::getLocalScale(transform);
//
//    if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
//    {
//        TransformHelper::setLocalPosition(transform, pos);
//        if (auto* collider = owner->GetComponent<ColliderComponent>())
//        {
//            collider->Recalculate(owner);
//        }
//    }
//
//    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
//    {
//        TransformHelper::setLocalRotation(transform, rot);
//        if (auto* collider = owner->GetComponent<ColliderComponent>())
//        {
//            collider->Recalculate(owner);
//        }
//    }
//
//    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.01f))
//    {
//        TransformHelper::setLocalScale(transform, scale);
//        if (auto* collider = owner->GetComponent<ColliderComponent>())
//        {
//            collider->Recalculate(owner);
//        }
//    }
//}
//
//void ShowRigidbodyEditor(RigidbodyComponent& rb)
//{
//    ImGui::Text("Rigidbody");
//
//    ImGui::DragFloat("Mass", &rb.mass, 0.01f, 0.0f, 1000.0f);
//    ImGui::DragFloat("Bounce", &rb.bounce, 0.01f, 0.0f, 1.0f);
//    ImGui::DragFloat("Angular Damping", &rb.angularDamping, 0.01f, 0.0f, 1.0f);
//
//    ImGui::Separator();
//    ImGui::Checkbox("Use Gravity", &rb.useGravity);
//    ImGui::Checkbox("Is Static", &rb.isStatic);
//
//    ImGui::Separator();
//    ImGui::DragFloat3("Velocity", &rb.velocity.x, 0.01f);
//    ImGui::DragFloat3("Acceleration", &rb.acceleration.x, 0.01f);
//    ImGui::DragFloat3("Angular Velocity", &rb.angularVelocity.x, 0.01f);
//    ImGui::DragFloat3("Torque", &rb.torque.x, 0.01f);
//
//    ImGui::Separator();
//    ImGui::BeginDisabled();
//    ImGui::DragFloat3("Physics Position", &rb.physicsPosition.x, 0.01f);
//    ImGui::DragFloat3("Previous Position", &rb.previousPosition.x, 0.01f);
//    ImGui::EndDisabled();
//
//    if (ImGui::Button("Reset Velocity")) {
//        rb.velocity = glm::vec3(0.0f);
//        rb.angularVelocity = glm::vec3(0.0f);
//        rb.torque = glm::vec3(0.0f);
//        rb.acceleration = glm::vec3(0.0f);
//    }
//}
//
//void ShowColliderEditor(ColliderComponent& col)
//{
//    ImGui::Text("Collider");
//    ImGui::DragFloat3("Offset", &col.offset.x, 0.01f);
//    ImGui::DragFloat3("HalfSize", &col.halfSize.x, 0.01f, 0.0f);
//    ImGui::Checkbox("Is Trigger", &col.isTrigger);
//    ImGui::Checkbox("Affects NavMesh", &col.affectsNavMesh);
//    ImGui::Checkbox("Is Walkable", &col.isWalkable);
//}
//
//void ShowLightEditor(LightComponent& light)
//{
//    ImGui::Text("Light");
//    ImGui::Checkbox("Enabled", &light.isOn);
//
//    const char* lightTypes[] = { "Directional", "Point", "Spot" };
//    int currentType = static_cast<int>(light.type);
//    if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
//        light.type = static_cast<LightType>(currentType);
//
//    ImGui::Separator();
//    ImGui::ColorEdit3("Ambient", &light.ambient.x);
//    ImGui::ColorEdit3("Diffuse", &light.diffuse.x);
//    ImGui::ColorEdit3("Specular", &light.specular.x);
//    ImGui::DragFloat("Intensity", &light.intensity, 0.001f, 0.0f, 1000.0f);
//    ImGui::Separator();
//
//    if (light.type == Point || light.type == Spot) {
//        ImGui::Text("Attenuation");
//        ImGui::DragFloat("Constant", &light.constant, 0.001f, 0.0f, 10.0f);
//        ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 10.0f);
//        ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 10.0f);
//        ImGui::DragFloat("Range", &light.range, 0.001f, 0.0f, 1000.0f);
//    }
//
//    if (light.type == Spot) {
//        ImGui::Separator();
//        ImGui::Text("Spotlight");
//
//        float innerAngle = glm::degrees(glm::acos(light.cutOff));
//        float outerAngle = glm::degrees(glm::acos(light.outerCutOff));
//
//        if (ImGui::DragFloat("Inner Cutoff", &innerAngle, 0.1f, 0.0f, 90.0f))
//            light.cutOff = glm::cos(glm::radians(innerAngle));
//
//        if (ImGui::DragFloat("Outer Cutoff", &outerAngle, 0.1f, 0.0f, 90.0f))
//            light.outerCutOff = glm::cos(glm::radians(outerAngle));
//    }
//}
//
//std::string OpenFileDialog()
//{
//    char filename[MAX_PATH] = "";
//
//    OPENFILENAMEA ofn = {};
//    ofn.lStructSize = sizeof(OPENFILENAMEA);
//    ofn.lpstrFile = filename;
//    ofn.nMaxFile = MAX_PATH;
//    ofn.lpstrInitialDir = "res";
//    ofn.lpstrFilter =
//        "Model Files\0*.obj;*.fbx;*.glb;*.gltf\0"
//        "All Files\0*.*\0";
//    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
//
//    if (GetOpenFileNameA(&ofn)) {
//        std::filesystem::path fullPath = filename;
//        std::filesystem::path projectRoot = std::filesystem::absolute("../../");
//        std::filesystem::path relative = std::filesystem::relative(fullPath, projectRoot);
//        std::string result = relative.string();
//        std::replace(result.begin(), result.end(), '\\', '/');
//        return result;
//    }
//    return "";
//}

//static std::unordered_map<std::string, Prefab> prefabs;
//
//void imgui_render(SceneManager& sceneManager)
//{
//    std::vector<std::pair<std::string, Prefab*>> availablePrefabs;
//    for (auto& [name, weakModel] : ResourceManager::Models)
//    {
//        if (!prefabs.contains(name))
//            prefabs.emplace(name, Prefab(weakModel));
//        availablePrefabs.push_back({ name, &prefabs.at(name) });
//    }
//
//    std::vector<GameObject*> sceneObjects;
//    Scene* activeScene = sceneManager.GetActiveScene();
//    activeScene->GetRoot()->TraverseChildren([&](GameObject* go) {
//        sceneObjects.push_back(go);
//        });
//    decorSystem.DrawImGui(availablePrefabs, sceneObjects, *activeScene, nullptr);
//    PlacementEditor::DrawImGui(*activeScene, availablePrefabs, selectedGameObject);
//    if (show_demo_window) {}
//
//    ImGui::Begin("Hello, world!");
//
//    if (ImGui::Button(wireframeMode ? "Switch to Fill Mode" : "Switch to Wireframe")) {
//        wireframeMode = !wireframeMode;
//        glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
//    }
//
//    ImGui::Separator();
//    ImGui::Text("Ambient");
//    ImGui::DragFloat("Ambient strength", &renderSystem->ambientStrength, 0.000001f, 0.0f, 1.0f, "%.6f");
//    if (activeScene->GetECS().GetSystem<RaycastSystem>())
//    {
//        ImGui::Text("Debug");
//        ImGui::Checkbox("Collider", &activeScene->GetECS().GetSystem<RaycastSystem>()->colliderDebug);
//    }
//    ImGui::Separator();
//    ImGui::Text("Hierarchy");
//    gameObjectFilter.Draw("Filtruj po nazwie", 200.0f);
//    ShowGameObjectTree(activeScene, sceneManager.GetActiveScene()->GetRoot(), gameObjectFilter);
//
//    if (selectedGameObject) {
//        ImGui::Separator();
//        ImGui::Text("Selected Entity: %s", selectedGameObject->name.c_str());
//
//
//        if (ImGui::BeginMenu("Add Component"))
//        {
//            if (ImGui::MenuItem("Rigidbody"))
//            {
//                if (!selectedGameObject->GetComponent<RigidbodyComponent>())
//                    selectedGameObject->AddComponent<RigidbodyComponent>();
//            }
//
//            if (ImGui::MenuItem("Collider"))
//            {
//                if (!selectedGameObject->GetComponent<ColliderComponent>())
//                    selectedGameObject->AddComponent<ColliderComponent>();
//            }
//
//            if (ImGui::MenuItem("Light"))
//            {
//                if (!selectedGameObject->GetComponent<LightComponent>())
//                    selectedGameObject->AddComponent<LightComponent>();
//            }
//
//            ImGui::EndMenu();
//        }
//
//        ShowTransformEditor(selectedGameObject, *selectedGameObject->GetComponent<TransformComponent>());
//
//        RigidbodyComponent* rb = selectedGameObject->GetComponent<RigidbodyComponent>();
//        if (rb != nullptr)
//            ShowRigidbodyEditor(*rb);
//
//        ColliderComponent* col = selectedGameObject->GetComponent<ColliderComponent>();
//        if (col != nullptr)
//            ShowColliderEditor(*col);
//
//        LightComponent* light = selectedGameObject->GetComponent<LightComponent>();
//        if (light != nullptr)
//            ShowLightEditor(*light);
//    }
//
//    if (ImGui::Button("Zapisz"))
//        sceneManager.Save();
//
//    if (ImGui::Button("Wczytaj"))
//        sceneManager.Load();
//
//    ImGui::Separator();
//    for (auto& [name, weakModel] : ResourceManager::Models) {
//        ImGui::PushID(name.c_str());
//        std::shared_ptr<Model> model = weakModel;
//        ImGui::Text("%s", name.c_str());
//        ImGui::SameLine();
//
//        if (!model) {
//            ImGui::TextDisabled("[loading]");
//        }
//        else {
//            if (!prefabs.contains(name))
//                prefabs.emplace(name, Prefab(model));
//
//            if (ImGui::Button("Instantiate")) {
//                Prefab& prefab = prefabs.at(name);
//                GameObject* obj = prefab.Instantiate(*activeScene, nullptr, nullptr);
//                if (obj) obj->name = name;
//            }
//        }
//        ImGui::PopID();
//    }
//
//    ImGui::End();
//
//
//
//    ImGui::Begin("Loaded Models");
//
//    if (ImGui::Button("Load Model")) {
//        std::string path = OpenFileDialog();
//        if (!path.empty())
//            ResourceManager::LoadModel(path);
//    }
//
//    if (ImGui::Button("Load asset")) {
//        std::string path = "res/Yaml/assets.yaml";
//        ResourceManager::LoadAssets(path);
//    }
//
//    if (ImGui::Button("Zapisz asset"))
//        ResourceManager::SaveAsset();
//
//    ImGui::Separator();
//
//    for (const auto& [name, weakModel] : ResourceManager::Models)
//        ImGui::Text("%s", name.c_str());
//
//    ImGui::End();
//
//
//    if (ImGui::Begin("Debug hizTexture")) {
//        auto system = sceneManager.GetActiveScene()->GetECS().GetSystem<RenderSystem>();
//        static int debugMip = 0;
//        ImGui::SliderInt("Mip", &debugMip, 0, 10);
//        int i = 0;
//        for (auto& [cam, hiz] : system->cameraHiZ) {
//            ImGui::Text("Kamera %d", i++);
//            if (hiz.hizTexture != 0)
//                system->ShowR32FTextureImGui(hiz.hizTexture, debugMip);
//            //system->drivenManager.DebugShadowMapImGui();
//        }
//    }
//    ImGui::End();
//
//    // Osobno � ca�y ImGui
//    //static int debugMip = 0;
//    //ImGui::Begin("HiZ Debug Controls");
//    //auto* renderer = system->drivenManager.GetRenderer(0);
//    //ImGui::SliderInt("Mip", &debugMip, 0, renderer->hizMipLevels - 1);
//    //ImGui::End();
//
//    //// Pobierz renderer �eby wywo�a� debug
//    //if (renderer) renderer->DebugShowHiZ(debugMip);
//
//    ImGui::Begin("Performance");
//
//    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
//    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
//
//    static bool vsyncEnabled = true;
//    if (ImGui::Checkbox("VSync", &vsyncEnabled))
//    {
//        glfwSwapInterval(vsyncEnabled ? 1 : 0);
//    }
//
//    if (ImGui::CollapsingHeader("CPU")) {
//        ImGui::Text("Total CPU: %.3f ms", perf.cpuFrameTime);
//        ImGui::Text("Input:     %.3f ms", perf.inputTime);
//        ImGui::Text("Logic:     %.3f ms", perf.logicTime);
//        ImGui::Text("Culling:   %.3f ms", renderSystem->stats.cullingTimeMs);
//        ImGui::Text("Draw prep: %.3f ms", renderSystem->stats.drawSubmitTimeMs);
//    }
//    if (ImGui::CollapsingHeader("GPU")) {
//        ImGui::Text("GPU Frame: %.3f ms", renderSystem->gpuQuery.getLastResult());
//    }
//    if (ImGui::CollapsingHeader("Render Stats")) {
//        ImGui::Text("Draw calls:    %d", renderSystem->stats.drawCalls);
//        ImGui::Text("Objects:       %d", renderSystem->stats.renderedObjects);
//        ImGui::Text("Triangles:     %d", renderSystem->stats.triangles);
//        ImGui::Text("State changes: %d", renderSystem->stats.stateChanges);
//    }
//    if (ImGui::CollapsingHeader("Culling")) {
//        ImGui::Checkbox("Frustum culling", &renderSystem->frustumCullingEnabled);
//        ImGui::Checkbox("Occlusion culling", &renderSystem->occlusionCullingEnabled);
//        ImGui::Text("Frustum culled:   %d", renderSystem->stats.frustumCulledSet.size());
//        ImGui::Text("Occlusion culled: %d", renderSystem->stats.occlusionCulledSet.size());
//    }
//    ImGui::PlotLines("Frame time", frameTimes, MAX_SAMPLES, index,
//        nullptr, 0.0f, 1.0f, ImVec2(0, 60));
//
//    ImGui::End();
//
//    ImGui::Begin("Puzzle Debug");
//
//    if (puzzleSlotsMap.empty()) {
//        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Brak slot�w puzzle");
//    }
//    else {
//        int i = 0;
//        for (auto& [slotGO, slot] : puzzleSlotsMap) {
//            ImGui::PushID(i++);
//
//            std::string slotName = slotGO ? slotGO->name : "???";
//            std::string expectedName = slot.expectedObject ? slot.expectedObject->name : "???";
//            std::string occupantName = slot.occupant ? slot.occupant->name : "(pusty)";
//
//            bool isEmpty = (slot.occupant == nullptr);
//            bool isCorrect = (!isEmpty && slot.occupant == slot.expectedObject);
//
//            // Kolor: zielony = dobry, czerwony = z�y, szary = pusty
//            ImVec4 color = isEmpty
//                ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f)   // szary
//                : isCorrect
//                ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) // zielony
//                : ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // czerwony
//
//            const char* status = isEmpty ? "[PUSTY]" : isCorrect ? "[OK]" : "[ZLE]";
//
//            ImGui::TextColored(color, "%s  Slot: %-20s  Oczekiwany: %-10s  Aktualny: %-10s",
//                status,
//                slotName.c_str(),
//                expectedName.c_str(),
//                occupantName.c_str()
//            );
//
//            ImGui::PopID();
//        }
//
//        ImGui::Separator();
//
//        // Podsumowanie
//        int correct = 0, filled = 0;
//        for (auto& [slotGO, slot] : puzzleSlotsMap) {
//            if (slot.occupant != nullptr) filled++;
//            if (slot.occupant != nullptr && slot.occupant == slot.expectedObject) correct++;
//        }
//        int total = (int)puzzleSlotsMap.size();
//
//        ImGui::Text("Wype�nione: %d / %d", filled, total);
//        ImGui::Text("Poprawne:   %d / %d", correct, total);
//
//        if (correct == total)
//            ImGui::TextColored(ImVec4(0, 1, 0, 1), ">> PUZZLE ROZWIAZANY! <<");
//    }
//
//    ImGui::Separator();
//    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Rozwiazanie Zagadek");
//
//    if (ImGui::Button("can_open_door_1 = true - Lazienka")) {
//        can_open_door_1 = true;
//    }
//
//    if (ImGui::Button("OnPuzzleSolved - Rentgen")) {
//        OnPuzzleSolved(sceneManager.GetActiveScene());
//    }
//
//    if (ImGui::Button("isPuzzleSolved = true - Krematorium")) {
//        crematoriumPuzzle.isPuzzleSolved = true;
//    }
//
//    if (ImGui::Button("Sprawn zebatka (main room)")) {
//        SpawnGearReward(sceneManager.GetActiveScene(), glm::vec3(28.0f, 3.0f, -183.0f), "Gear_CheatSpawn_" + std::to_string(glfwGetTime()));
//    }
//
//    ImGui::Separator();
//    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Tuning Trzymanej Zebatki");
//    ImGui::DragFloat3("Offset w reku", &gearHeldOffset.x, 0.05f);
//    ImGui::DragFloat3("Rotacja w reku", &gearHeldRotation.x, 1.0f);
//
//    ImGui::End();
//}
//
//void imgui_end()
//{
//    ImGui::Render();
//    int display_w, display_h;
//    glfwMakeContextCurrent(window);
//    glfwGetFramebufferSize(window, &display_w, &display_h);
//    glViewport(0, 0, display_w, display_h);
//    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}
