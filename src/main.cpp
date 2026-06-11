// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

//#include "imgui.h"
//#include "imgui_impl/imgui_impl_glfw.h"
//#include "imgui_impl/imgui_impl_opengl3.h"
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
//#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <unused/camera.h>
#include <model.h>
#include <prefab.h>
#include <filesystem>
#include <optional>

#include <fmod.h>
#include <fmod.hpp>

#include <systems/HID.h>
#include <freetype/freetype.h>
#include <yaml-cpp/binary.h>
#include "yaml_config.h"

#include <core/scene.h>
#include <core/scene_manager.h>
#include "core/gameobject.h"
#include <systems/physics_system.h>
#include <systems/transform_system.h>
#include <systems/animation_system.h>
#include <systems/SpriteSystem.h>
#include <systems/raycastSystem.h>
#include <systems/AudioSystem.h>

#include <systems/NavMeshSystem.h>
#include "diagnostics/cpu_timer.h"

#include "systems/PostProcessingSystem.h"
#include "systems/NavPathSystem.h"
#include "systems/NpcSystem.h"
#include "utils/render_helper.h"
#include "utils/animation_helper.h"
#include "utils/player_animation_helper.h"

#include "gameplay/crematorium_puzzle.h"


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool init();
//void init_imgui();

void compileShader();

void input();
void update();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

bool processCameraInput(ECS& ecs, CameraComponent& cam, TransformComponent& playerTransform,
    const std::string& up,
    const std::string& down,
    const std::string& left,
    const std::string& right);

bool processCameraGamepad(ECS& ecs, CameraComponent& cam, TransformComponent& transformCamera, TransformComponent& playerTransform, int gamepad_id);

void imgui_begin();
void imgui_render(SceneManager& sceneManager);
void imgui_end();

void end_frame();

constexpr int32_t WINDOW_WIDTH  = 1920;
constexpr int32_t WINDOW_HEIGHT = 1080;

GLFWwindow* window = nullptr;

// Change these to lower GL version like 4.5 if GL 4.6 can't be initialized on your machine
const     char* glsl_version       = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

// camera
float lastX     = WINDOW_WIDTH  / 2.0f;
float lastY     = WINDOW_HEIGHT / 2.0f;
bool  firstMouse = true;
bool  mouseMove  = false;
glm::vec3 cameraOffset = glm::vec3(0.0f, 10.0f, -20.0f);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool   show_demo_window    = true;
bool   show_another_window = false;
bool   wireframeMode       = false;
bool   focused             = false;
int    sphereRings         = 10;
int    sphereSectors       = 10;
float  sphereRadius        = 1.0f;

float  cameraDistance = 50.0f;
float  rotationX      = 0.0f;
float  rotationY      = 0.0f;

ImVec4 clear_color   = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
bool   autoRotation  = false;

unsigned int cubemapTexture;
unsigned int skyboxVAO;

CrematoriumPuzzle crematoriumPuzzle;

GLuint VBO;
GLuint VAO;
GLuint texture;
std::unique_ptr<Prefab> sunModel;
std::unique_ptr<Shader> skyboxShader;
//std::unique_ptr<Shader> reflectShader;
//std::unique_ptr<Shader> refractShader;

std::unique_ptr<Prefab> postacGracza;
std::unique_ptr<Prefab> bed1Model;
std::unique_ptr<Prefab> bed2Model;
std::unique_ptr<Prefab> bed3Model;
std::unique_ptr<Prefab> deskModel;
std::unique_ptr<Prefab> doorsModel;
std::unique_ptr<Prefab> krzesloModel;
std::unique_ptr<Prefab> lampa1Model;
std::unique_ptr<Prefab> lampa2Model;
std::unique_ptr<Prefab> lampa3Model;
std::unique_ptr<Prefab> needleModel;
std::unique_ptr<Prefab> bad1Model;
std::unique_ptr<Prefab> bad2Model;
std::unique_ptr<Prefab> bad3Model;
std::unique_ptr<Prefab> bossModel;
std::unique_ptr<Prefab> bossCapsuleModel;
std::unique_ptr<Prefab> characterModel;
std::unique_ptr<Prefab> vial1Model;
std::unique_ptr<Prefab> vial2Model;
std::unique_ptr<Prefab> vial3Model;
std::unique_ptr<Prefab> vial4Model;
std::unique_ptr<Prefab> vial5Model;
std::unique_ptr<Prefab> vial61Model;
std::unique_ptr<Prefab> vial7Model;
std::unique_ptr<Prefab> sinkModel;
std::unique_ptr<Prefab> szafa1Model;
std::unique_ptr<Prefab> szafa2Model;
std::unique_ptr<Prefab> szafa3Model;
std::unique_ptr<Prefab> telephoneModel;
std::unique_ptr<Prefab> toiletModel;
std::unique_ptr<Prefab> wozekModel;
std::unique_ptr<Prefab> zaslonaModel;
std::unique_ptr<Prefab> roomModel;
std::unique_ptr<Prefab> placeholderModel;
std::unique_ptr<Prefab> doorsToiletModel;
std::unique_ptr<Prefab> toiletPaperRedModel;
std::unique_ptr<Prefab> toiletPaperGreenModel;
std::unique_ptr<Prefab> toiletPaperBlueModel;
std::unique_ptr<Prefab> mirrorModel1;
std::unique_ptr<Prefab> mirrorModel2;
std::unique_ptr<Prefab> mirrorModel3;
std::unique_ptr<Prefab> mirrorModel4;
std::unique_ptr<Prefab> washroomExit;
std::unique_ptr<Prefab> urinModel;
std::unique_ptr<Prefab> szafkaModel;
std::unique_ptr<Prefab> ruraModel;
std::unique_ptr<Prefab> panelModel;
std::unique_ptr<Prefab> floorModel;
std::unique_ptr<Prefab> wallModel;
std::unique_ptr<Prefab> wallModel2;
std::unique_ptr<Prefab> wallModel3;
std::unique_ptr<Prefab> NormalDoor;
std::unique_ptr<Prefab> cockroachModel;

std::unique_ptr<Prefab> puzel1;
std::unique_ptr<Prefab> puzel2;
std::unique_ptr<Prefab> puzel3;
std::unique_ptr<Prefab> puzel4;
std::unique_ptr<Prefab> puzel5;
std::unique_ptr<Prefab> puzel6;

std::unique_ptr<Prefab> czerwonaTablica;
std::unique_ptr<Prefab> zielonaTablica;
std::unique_ptr<Prefab> Rentgen;

std::unique_ptr<Prefab> kredensModel;
std::unique_ptr<Prefab> eksp1Model;
std::unique_ptr<Prefab> fiolka2Model;
std::unique_ptr<Prefab> fiolka1Model;
std::unique_ptr<Prefab> ksiazkaModel;
std::unique_ptr<Prefab> eksp2Model;

std::unique_ptr<Prefab> probowka7Model;
std::unique_ptr<Prefab> probowka6Model;
std::unique_ptr<Prefab> probowka5Model;
std::unique_ptr<Prefab> probowkaArka_1_Model;
std::unique_ptr<Prefab> probowka3Model;
std::unique_ptr<Prefab> probowka4Model;
std::unique_ptr<Prefab> labOla1Model;
std::unique_ptr<Prefab> probowka2Model;
std::unique_ptr<Prefab> folderModel;
std::unique_ptr<Prefab> papersModel;
std::unique_ptr<Prefab> cupModel;
std::unique_ptr<Prefab> corkBoardModel;
std::unique_ptr<Prefab> clockModel;
std::unique_ptr<Prefab> computer_pbrModel;
std::unique_ptr<Prefab> laboratoryStuff1Model;
std::unique_ptr<Prefab> laboratoryStuff2Model;
std::unique_ptr<Prefab> laboratoryStuff3Model;

std::unique_ptr<Prefab> dyingModelPrefab;
std::unique_ptr<Prefab> jumpSkeletonPrefab;

// testowe obiekty do postprocessingu
std::unique_ptr<Prefab> RedModel;
std::unique_ptr<Prefab> BlueModel;
std::unique_ptr<Prefab> GreenModel;

std::unique_ptr<Prefab> roofModel;
std::unique_ptr<Prefab> groundModel;

std::unique_ptr<Prefab> koparkaModel;

unsigned int triangleVAO = 0;
unsigned int triangleVBO = 0;
std::unique_ptr<Shader> triangleShader;

const int MAX_SAMPLES = 100;
float frameTimes[MAX_SAMPLES];
int index = 0;

struct PerformanceData {
    float cpuFrameTime = 0.0f;
    float logicTime    = 0.0f;
    float inputTime    = 0.0f;
};
PerformanceData perf;
RenderSystem*          renderSystem          = nullptr;
PostProcessingSystem*  postProcessingSystem  = nullptr;

// needed for interaction
GameObject* tablicaPapierowKibel[6];
std::unordered_set<GameObject*> rotatableObjects;
std::unordered_set<GameObject*> unlockedDoors;
std::unordered_set<GameObject*> majorDoors;
bool can_open_door_1 = true;

bool isCabinetButtonPushed = false;

struct CabinetState {
    bool isOpen       = false;
    float currentAngle = 0.0f;
    float targetAngle  = 0.0f;
    GameObject* leftDoor  = nullptr;
    GameObject* rightDoor = nullptr;
    GameObject* button    = nullptr;
    glm::vec3 buttonStartPos;
    glm::vec3 buttonTargetPos;
};

struct DoorState {
    bool  isOpen       = false;
    float currentAngle = 0.0f;
    float closedAngle  = 0.0f;
    float openAngle    = 90.0f;
    float targetAngle  = 0.0f;
    GameObject* hinge  = nullptr;
    glm::vec3 originalOffset = glm::vec3(0.0f);
    bool canBeClicked  = true;
};

std::unordered_map<GameObject*, CabinetState> cabinetsMap;
std::unordered_map<GameObject*, DoorState>    toiletDoorsMap;
std::unordered_set<GameObject*>               pickupObjects;
GameObject* p1HeldObject = nullptr;
GameObject* p2HeldObject = nullptr;

std::vector<GameObject*> mainRoomDoors;

//interfejs type situation
float p1ShakeTimer = 0.0f;
float p2ShakeTimer = 0.0f;
const float SHAKE_DURATION = 0.5f;
const glm::vec2 p1BasePos(480.0f, 640.0f);
const glm::vec2 p2BasePos(1440.0f, 640.0f);

void updateFPS(float deltaTime) {
    frameTimes[index] = deltaTime;
    index = (index + 1) % MAX_SAMPLES;

    float sum = 0.0f;
    for (int i = 0; i < MAX_SAMPLES; i++)
        sum += frameTimes[i];

    float avg = sum / MAX_SAMPLES;
    float fps = 1.0f / avg;
}

void updateFocus() {
    if (focused)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!focused)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

GameObject* CreateRaycastTestObject(
    Scene& scene,
    Prefab& prefab,
    Shader* shader,
    const glm::vec3& position,
    const glm::vec3& scale = glm::vec3(1.0f)
) {
    GameObject* go = prefab.Instantiate(scene, nullptr, shader);

    auto* tr      = go->GetComponent<TransformComponent>();
    tr->position  = position;
    tr->scale     = scale;
    tr->isDirty   = true;

    auto* col     = go->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(1.0f);
    col->offset   = glm::vec3(0.0f);

    auto* rc          = go->AddComponent<RaycastComponent>();
    rc->range         = 5000.0f;
    rc->fovRayCount   = 200;
    rc->fovAngle      = 200.0f;
    rc->originOffset  = glm::vec3(0.0f, 0.5f, 0.0f);
    rc->debugDraw     = false;

    return go;
}


bool processCameraInput(ECS& ecs, CameraComponent& cam, TransformComponent& playerTransform,
    const std::string& up,
    const std::string& down,
    const std::string& left,
    const std::string& right)
{
    const auto& hid = ecs.GetSystem<HID>();
    glm::vec3 dir(0.0f);

    glm::vec3 camFront = cam.state.Front;
    camFront.y = 0.0f;
    camFront = glm::normalize(camFront);

    glm::vec3 camRight = cam.state.Right;
    camRight.y = 0.0f;
    camRight = glm::normalize(camRight);

    if (hid->is_action_pressed(up))    dir += camFront;
    if (hid->is_action_pressed(down))  dir -= camFront;
    if (hid->is_action_pressed(left))  dir -= camRight;
    if (hid->is_action_pressed(right)) dir += camRight;

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        playerTransform.position += dir * MovementSpeed * 0.04f; //deltaTime; aktualnie fixedDeltaTime
        playerTransform.isDirty = true;
        cam.dirty = true;
        return true;
    }
    return false;
}


const float sensitivityCamera = 1200.0f;

void processCameraMouse(ECS& ecs, CameraComponent& cam, TransformComponent& transformCamera, TransformComponent& playerTransform)
{
    const auto& hid = ecs.GetSystem<HID>();
    float dx = hid->get_mouse_dx();
    float dy = hid->get_mouse_dy();
    const float epsilon = 0.01f;

    if (glm::abs(dx) < epsilon && glm::abs(dy) < epsilon)
        return;

    playerTransform.rotation.y -= dx * sensitivityCamera / 600.0f * 0.04f;// deltaTime; aktualnie fixedDeltaTime
    playerTransform.isDirty = true;

    CameraHelper::ProcessMouseMovement(cam, transformCamera, 0.0f, dy);
}

float lookDeadzone = 0.0f;

bool processCameraGamepad(ECS& ecs, CameraComponent& cam, TransformComponent& transformCamera, TransformComponent& playerTransform, int gamepad_id)
{
    const auto& hid = ecs.GetSystem<HID>();

    float lx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_X, gamepad_id);
    float ly = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_Y, gamepad_id);

    glm::vec3 dir(0.0f);

    glm::vec3 camFront = cam.state.Front;
    camFront.y = 0.0f;
    camFront = glm::normalize(camFront);

    glm::vec3 camRight = cam.state.Right;
    camRight.y = 0.0f;
    camRight = glm::normalize(camRight);

    dir += camFront * (-ly);
    dir += camRight * lx;

    bool isMoving = false;
    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        playerTransform.position += dir * MovementSpeed * 0.04f;// deltaTime; aktualnie fixedDeltaTime
        playerTransform.isDirty = true;
        cam.dirty = true;
        isMoving = true;
    }

    float rx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_X, gamepad_id);
    float ry = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_Y, gamepad_id);


    if (lookDeadzone <= 0.01f)
        lookDeadzone += 0.0005f;
    else if (glm::abs(rx) < lookDeadzone && glm::abs(ry) < lookDeadzone)
        return isMoving;

    playerTransform.rotation.y -= rx * sensitivityCamera / 10.0f * deltaTime;// deltaTime; aktualnie fixedDeltaTime
    playerTransform.isDirty = true;

    CameraHelper::ProcessMouseMovement(cam, transformCamera, 0.0f, ry * sensitivityCamera * deltaTime);
    return isMoving;
}


void addAllSystems(ECS& ecs);
void connectAllModels();
void LoadPlayerAnimations();

struct PuzzleSlot {
    glm::vec3 targetRotation;
    GameObject* occupant       = nullptr;
    GameObject* slotObject     = nullptr;
    GameObject* expectedObject = nullptr;
    GameObject* lightObject    = nullptr;
};

GameObject* puzzleRewardObject = nullptr;


void OnPuzzleSolved(Scene* scene) {
    spdlog::info("Puzzle rozwiązany!");

    // Zabezpieczenie przed wielokrotnym wywołaniem
    if (puzzleRewardObject != nullptr) return;

    if (placeholderModel == nullptr) {
        spdlog::error("placeholderModel nie zaladowany!");
        return;
    }

    puzzleRewardObject = placeholderModel->Instantiate(*scene, nullptr, nullptr);
    puzzleRewardObject->name = "PuzzleReward";

    TransformComponent* tr = puzzleRewardObject->GetComponent<TransformComponent>();
    tr->position = glm::vec3(0.0f, 5.0f, -270.0f);
    tr->scale    = glm::vec3(3.0f);
    tr->isDirty  = true;

    RigidbodyComponent* rb = puzzleRewardObject->AddComponent<RigidbodyComponent>();
    rb->useGravity = false;
    rb->isStatic   = true;

    puzzleRewardObject->AddComponent<ColliderComponent>();
}

std::unordered_map<GameObject*, PuzzleSlot> puzzleSlotsMap; // klucz = slotObject
std::unordered_map<GameObject*, glm::vec3>  objectOriginalRotations;

bool IsPuzzleSolved() {
    if (puzzleSlotsMap.empty()) return false;
    for (auto& [slotGO, slot] : puzzleSlotsMap) {
        if (slot.occupant == nullptr) return false;             // slot pusty
        if (slot.occupant != slot.expectedObject) return false; // zła kostka
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
    Prefab* playerPrefab
) {
    if (!ecs.GetSystem<HID>()->is_action_just_pressed(inputAction)) return;
    //upuszczanie
    if (myHeldObject != nullptr) {
        TransformComponent* camTr   = playerCamera->GetComponent<TransformComponent>();
        TransformComponent* heldTr  = myHeldObject->GetComponent<TransformComponent>();
        CameraComponent*    camComp = playerCamera->GetComponent<CameraComponent>();

        // Sprawdź czy raycast widzi slot
        PuzzleSlot* targetSlot = nullptr;
        if (playerRaycast->anyHit()) {
            RaycastHit hit = playerRaycast->closestHit();
            if (hit.hitObject && puzzleSlotsMap.count(hit.hitObject)) {
                PuzzleSlot& slot = puzzleSlotsMap[hit.hitObject];
                if (slot.occupant == nullptr)   // tylko wolny slot
                    targetSlot = &slot;
            }
        }

        myHeldObject->SetParent(scene->GetRoot());

        if (targetSlot != nullptr) {
            // ── Gracz patrzy na wolny slot ──
            TransformComponent* slotTr = targetSlot->slotObject->GetComponent<TransformComponent>();
            heldTr->position = slotTr->position;
            heldTr->rotation = targetSlot->targetRotation;
            heldTr->isDirty  = true;
            targetSlot->occupant = myHeldObject;
            if (IsPuzzleSolved())
                OnPuzzleSolved(scene);

            if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                rb->useGravity = false;
                rb->isStatic   = true;
                rb->velocity = glm::vec3(0.0f);
                rb->acceleration = glm::vec3(0.0f);
            }
        }
        else {
            // ── Gracz patrzy gdzie indziej — normalne upuszczenie ──
            heldTr->position = TransformHelper::getGlobalPosition(*camTr) + (camComp->state.Front * 5.0f);
            heldTr->rotation = objectOriginalRotations.count(myHeldObject)
                                   ? objectOriginalRotations[myHeldObject]
                                   : glm::vec3(0.0f);
            heldTr->isDirty  = true;

            if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                rb->useGravity = true;
                rb->isStatic   = false;
                rb->previousPosition = heldTr->position;
                rb->physicsPosition = heldTr->position;
                rb->velocity   = glm::vec3(0.0f);
                rb->acceleration   = glm::vec3(0.0f);
            }
        }

        myHeldObject = nullptr;
    }
    else if (playerRaycast->anyHit()) {
        RaycastHit hit = playerRaycast->closestHit();
        if (hit.hitObject != nullptr) {
            // Obracanie
            if (rotatableObjects.count(hit.hitObject)) {
                if (!rotatingInProgress.count(hit.hitObject)) {
                    rotatingObjects[hit.hitObject] = 60.0f;
                    rotatingInProgress.insert(hit.hitObject);
                    PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);
                }
            }
            // PAIN
            if (majorDoors.count(hit.hitObject)) {
                if (can_open_door_1) {
                    for (GameObject* door : majorDoors) {
                        TransformComponent* t = door->GetComponent<TransformComponent>();
                        if (t) t->position = glm::vec3(-1000.0f, -1000.0f, -1000.0f);
                    }
                } else {
                    outShakeTimer = SHAKE_DURATION;
                }
                PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);
            }
            // Otwieranie drzwi
            else if (toiletDoorsMap.count(hit.hitObject)) {
                DoorState& state = toiletDoorsMap[hit.hitObject];
                if (state.canBeClicked) {
                    state.isOpen      = !state.isOpen;
                    state.targetAngle = state.isOpen ? state.openAngle : state.closedAngle;

                    if (auto col = hit.hitObject->GetComponent<ColliderComponent>()) {
                        col->halfSize = state.isOpen ? glm::vec3{ 1.0f, 10.0f, 1.0f } : glm::vec3{ 0.8f, 10.0f, 4.0f };
                        col->offset   = state.isOpen ? glm::vec3(0.0f) : state.originalOffset;
                    }
                    PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::INTERACT_ANIM_INDEX);
                }
            }
            // Otwieranie szafki
            else if (cabinetsMap.count(hit.hitObject)) {
                if (!isCabinetButtonPushed) {
                    isCabinetButtonPushed = true;
                    CabinetState& state   = cabinetsMap[hit.hitObject];
                    state.isOpen          = true;
                    state.targetAngle     = 90.0f;

                    for (GameObject* hinge : mainRoomDoors) {
                        if (hinge && toiletDoorsMap.count(hinge)) {
                            DoorState& dState  = toiletDoorsMap[hinge];
                            dState.isOpen      = true;
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
                    heldTr->rotation = glm::vec3(0.0f);
                    heldTr->isDirty  = true;

                    if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                        rb->useGravity = false;
                        rb->isStatic   = true;
                    }

                    PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::PICKUP_ANIM_INDEX);
                }
            }
            // Podnoszenie (zabezpieczone przed wyrwaniem obiektu drugiemu graczowi)
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
                heldTr->rotation = glm::vec3(0.0f);
                heldTr->isDirty  = true;

                if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                    rb->useGravity = false;
                    rb->isStatic   = true;
                }

                PlayerAnimationHelper::TriggerAction(playerAnimator, playerPrefab, PlayerAnimationHelper::PICKUP_ANIM_INDEX);
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
    std::unordered_set<GameObject*>& rotatingInProgress
) {
    if (!ecs.GetSystem<HID>()->is_action_just_pressed(inputAction)) return;
    if (!playerRaycast->anyHit()) return;

    RaycastHit hit = playerRaycast->closestHit();
    if (hit.hitObject && rotatableObjects.count(hit.hitObject)) {
        if (!rotatingInProgress.count(hit.hitObject)) {
            rotatingObjects[hit.hitObject] = -60.0f;
            rotatingInProgress.insert(hit.hitObject);
        }
    }
}

void UpdateDoors(float deltaTime) {
    float doorAnimSpeed = 180.0f;
    for (auto& [doorObj, state] : toiletDoorsMap) {
        if (std::abs(state.currentAngle - state.targetAngle) > 0.1f) {
            float direction = (state.targetAngle > state.currentAngle) ? 1.0f : -1.0f;
            state.currentAngle += direction * doorAnimSpeed * deltaTime;

            if ((direction > 0.0f && state.currentAngle > state.targetAngle) ||
                (direction < 0.0f && state.currentAngle < state.targetAngle)) {
                state.currentAngle = state.targetAngle;
            }

            TransformComponent* hingeTr = state.hinge->GetComponent<TransformComponent>();
            if (hingeTr) {
                hingeTr->rotation.y = state.currentAngle;
                hingeTr->isDirty = true;
            }
        }
    }
}

void UpdateCabinets(float deltaTime) {
    float animSpeed   = 180.0f;
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
    float baseRotationY = 90.0f)
{
    GameObject* hinge  = scene->CreateGameObject(nullptr);
    hinge->name        = "Hinge_" + name;
    TransformComponent* hingeTr = hinge->AddComponent<TransformComponent>();
    hingeTr->position  = position + pivotOffset;

    GameObject* door   = prefab->Instantiate(*scene, hinge, shader);
    door->name         = name;
    TransformComponent* doorTr = door->GetComponent<TransformComponent>();
    doorTr->scale      = scale;
    doorTr->rotation   = glm::vec3(0.0f, baseRotationY, 0.0f);
    doorTr->position   = -pivotOffset;


    ColliderComponent* col = hinge->AddComponent<ColliderComponent>();
    col->halfSize       = colliderHalfSize;
    col->offset         = -pivotOffset;
    col->isWalkable     = false;
    col->affectsNavMesh = true;

    DoorState state;
    state.hinge          = hinge;
    state.openAngle      = openAngle;
    state.closedAngle    = 0.0f;
    state.currentAngle   = 0.0f;
    state.targetAngle    = 0.0f;
    state.originalOffset = -pivotOffset;
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
    std::optional<glm::vec3> rotation         = std::nullopt,
    std::optional<glm::vec3> colliderHalfSize = std::nullopt,
    bool affectsNavMesh = false
) {
    GameObject* go = prefab->Instantiate(*scene, nullptr, shader);
    go->name       = name;

    TransformComponent* tr = go->GetComponent<TransformComponent>();
    if (tr) {
        tr->position = position;
        tr->scale    = scale;
        if (rotation.has_value())
            tr->rotation = rotation.value();
        tr->isDirty  = true;
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
    go->name       = "CockroachLeader";

    auto* tr      = go->GetComponent<TransformComponent>();
    tr->position  = homePos;
    tr->scale     = glm::vec3(0.3f);
    tr->isDirty   = true;

    auto* col      = go->AddComponent<ColliderComponent>();
    col->halfSize  = glm::vec3(0.3f, 0.2f, 0.3f);

    auto* nav         = go->AddComponent<NavPathComponent>();
    nav->state        = NavAgentState::ExternalControl;
    nav->moveSpeed    = moveSpeed;
    nav->idleTimeMax  = 0.0f;

    auto* leader                = go->AddComponent<CockroachLeaderComponent>();
    leader->homePosition        = homePos;
    leader->homeRadius          = 15.0f;
    leader->homeTimeRequired    = 8.0f;
    leader->exploreRadius       = 50.0f;
    leader->exploreDuration     = 20.0f;
    leader->detectionRadius     = 25.0f;
    leader->escapeRadius        = 35.0f;
    leader->idleWanderRadius    = 8.0f;
    leader->state               = LeaderState::Idle;

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
    go->name       = "CockroachFollower";

    auto* tr      = go->GetComponent<TransformComponent>();
    tr->position  = spawnPos;
    tr->scale     = glm::vec3(0.25f);
    tr->isDirty   = true;


    auto* col      = go->AddComponent<ColliderComponent>();
    col->halfSize  = glm::vec3(0.25f, 0.15f, 0.25f);

    auto* nav        = go->AddComponent<NavPathComponent>();
    nav->state       = NavAgentState::ExternalControl;
    nav->moveSpeed   = moveSpeed;
    nav->idleTimeMax = 0.0f;

    auto* follower                  = go->AddComponent<CockroachFollowerComponent>();
    follower->leaderGameObject      = leaderGO;
    follower->followDistance        = 6.0f;
    follower->followStopDistance    = 2.0f;
    follower->idleWanderRadius      = 6.0f;
    follower->state                 = FollowerState::Follow;

    return go;
}

void createFirstRoom(Scene* scena1);
void createMainRooom(Scene* scena);
void createNuclearRooom(Scene* scena);
void createCrematorium(Scene* scena);
void createRentgenRoom(Scene* scena);

int main(int, char**)
{
    if (!init())
    {
      //  spdlog::error("Failed to initialize project!");
        return EXIT_FAILURE;
    }
    //spdlog::info("Initialized project.");

//    init_imgui();
    //spdlog::info("Initialized ImGui.");

    ECS ecs;
    SceneManager sceneManager;

    sceneManager.CreateScene("Scena 1", ecs);

    Scene* scena1 = sceneManager.GetActiveScene();

    addAllSystems(ecs);

    postacGracza = std::make_unique<Prefab>("res/models/postac_test.glb");
    groundModel = std::make_unique<Prefab>("res/models/podloze.glb");
    //sunModel    = std::make_unique<Prefab>("res/models/Sun.glb");

    //GameObject* obb3 = sunModel->Instantiate(*scena1, nullptr, nullptr);
    //obb3->GetComponent<TransformComponent>()->scale    = glm::vec3(25.0f);
    //obb3->GetComponent<TransformComponent>()->position = glm::vec3(75.0f, 250.0f, 0.0f);

    //obb3->AddComponent<RigidbodyComponent>()->useGravity = false;
    //obb3->AddComponent<ColliderComponent>()->halfSize    = glm::vec3{ 25, 25, 25 };

    //GLuint diff = ResourceManager::LoadTexture("diffuse_brick.png",  "res/textures/").id;
    //GLuint spec = ResourceManager::LoadTexture("specular_brick.png", "res/textures/").id;
    //GLuint norm = ResourceManager::LoadTexture("normal_brick.png",   "res/textures/").id;

    //auto brickMat           = std::make_shared<Material>();
    //brickMat->shader        = nullptr;
    //brickMat->diffuseMap    = diff;
    //brickMat->specularMap   = spec;
    //brickMat->normalMap     = norm;
    //brickMat->shininess     = 64.0f;

    //RenderHelper::SetMaterial(obb3, brickMat);

    //Tworzenie gracza nr.1
    GameObject* gracz1 = scena1->CreateGameObject(nullptr);
    gracz1->name = "Gracz1";

    ColliderComponent* camera1collider = gracz1->AddComponent<ColliderComponent>();
    RigidbodyComponent* rigidBodyCamera1 = gracz1->AddComponent<RigidbodyComponent>();
    gracz1->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 20.0f, -20.0f);
    gracz1->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
    gracz1->GetComponent<RigidbodyComponent>()->mass = 10.0f;
    gracz1->GetComponent<RigidbodyComponent>()->bounce = 0.1f;
    gracz1->GetComponent<RigidbodyComponent>()->useGravity = true;
    gracz1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.0f, 5.25f, 1.0f };

    
    GameObject* camera1 = scena1->CreateGameObject(nullptr);//groundModel->Instantiate(*scena1, nullptr, ourShader.get());
    camera1->name = "Kamera";
    gracz1->AddChild(camera1);
    camera1->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 4.7f, 0.0f);
    camera1->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
    CameraComponent* camCompLeft = camera1->AddComponent<CameraComponent>();
    RaycastComponent*  player1Raycast   = camera1->AddComponent<RaycastComponent>();
    player1Raycast->debugDraw = false;

    camera1->AddComponent<LightComponent>();
    LightComponent* light2 = camera1->GetComponent<LightComponent>();

    light2->type      = Spot;
    light2->index     = 0;
    light2->ambient   = glm::vec3(0.25f);
    light2->diffuse   = glm::vec3(1.0f);
    light2->specular  = glm::vec3(1.0f);
    light2->constant  = 1.0f;
    light2->linear    = 0.10f;
    light2->quadratic = 0.00001f;
    light2->intensity = 450.0f;
    light2->cutOff      = glm::cos(glm::radians(8.0f));
    light2->outerCutOff = glm::cos(glm::radians(22.0f));

    GameObject* modelPostac1 = postacGracza->Instantiate(*scena1, nullptr, nullptr);
    gracz1->AddChild(modelPostac1);
    modelPostac1->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 0.9f, -1.7f);
    AnimatorComponent* p1Animator = modelPostac1->GetComponent<AnimatorComponent>();
    if (p1Animator == nullptr) {
        p1Animator = modelPostac1->AddComponent<AnimatorComponent>();
        p1Animator->currentSkeleton = &postacGracza->rootModel->skeleton;
    }

    //Tworzenie gracza nr.2
    GameObject* gracz2 = scena1->CreateGameObject(nullptr);
    gracz2->name = "Gracz2";

    ColliderComponent*  camera2collider  = gracz2->AddComponent<ColliderComponent>();
    RigidbodyComponent* rigidBodyCamera2 = gracz2->AddComponent<RigidbodyComponent>();
    gracz2->GetComponent<TransformComponent>()->position = glm::vec3(-10.0f, 20.0f, -20.0f);
    gracz2->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
    gracz2->GetComponent<RigidbodyComponent>()->mass = 10.0f;
    gracz2->GetComponent<RigidbodyComponent>()->bounce = 0.1f;
    gracz2->GetComponent<RigidbodyComponent>()->useGravity = true;
    gracz2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.0f, 5.25f, 1.0f };

    GameObject* camera2 = scena1->CreateGameObject(nullptr);
    camera2->name = "Kamera";
    gracz2->AddChild(camera2);
    camera2->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 4.7f, 0.0f);
    camera2->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    CameraComponent* camCompRight = camera2->AddComponent<CameraComponent>();
    RaycastComponent* player2Raycast = camera2->AddComponent<RaycastComponent>();
    player2Raycast->debugDraw = false;

    camera2->AddComponent<LightComponent>();
    LightComponent* light3 = camera2->AddComponent<LightComponent>();
    light3->type = Spot;
    light3->index = 2;
    light3->ambient = glm::vec3(0.25f);
    light3->diffuse = glm::vec3(1.0f);
    light3->specular = glm::vec3(1.0f);
    light3->constant = 1.0f;
    light3->linear = 0.10f;
    light3->quadratic = 0.00001f;
    light3->intensity = 450.0f;
    light3->cutOff = glm::cos(glm::radians(8.0f));
    light3->outerCutOff = glm::cos(glm::radians(22.0f));

    GameObject* modelPostac2 = postacGracza->Instantiate(*scena1, nullptr, nullptr);
    gracz2->AddChild(modelPostac2);
    modelPostac2->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 0.9f, 1.7f);
	modelPostac2->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    AnimatorComponent* p2Animator = modelPostac2->GetComponent<AnimatorComponent>();
    if (p2Animator == nullptr) {
        p2Animator = modelPostac2->AddComponent<AnimatorComponent>();
        p2Animator->currentSkeleton = &postacGracza->rootModel->skeleton;
    }
    
    auto* t0 = gracz1->GetComponent<TransformComponent>();
    auto* t1 = gracz2->GetComponent<TransformComponent>();    
    //Zakoniczenie tworzenia postaci

    //Kamery postaci
    TransformComponent* camTransform1 = camera1->GetComponent<TransformComponent>();
    CameraHelper::InitialCamera(*camCompLeft, *camTransform1,
        glm::vec3(0.0f, 1.0f, 0.0f),
        //YAW, PITCH,
        Viewport{ 0.0f, 0.0f, 0.5f, 1.0f }
    );
    camCompLeft->isActive = true;

    TransformComponent* camTransform2 = camera2->GetComponent<TransformComponent>();
    CameraHelper::InitialCamera(*camCompRight, *camTransform2,
        glm::vec3(0.0f, 1.0f, 0.0f),
        //0.0f, -20.0f,
        Viewport{ 0.5f, 0.0f, 0.5f, 1.0f }
    );
    camCompRight->isActive = true;

    GameObject* player1InteractionInfo_obj = scena1->CreateGameObject(nullptr);
    SpriteComponent* player1InteractionInfo = player1InteractionInfo_obj->AddComponent<SpriteComponent>();
    player1InteractionInfo->textEnabled        = true;
    player1InteractionInfo->screenPosition     = glm::vec2(480.0f, 640.0f);
    player1InteractionInfo->text               = "";
    player1InteractionInfo->textOutlineEnabled = true;
    player1InteractionInfo->textCentered       = true;
    player1InteractionInfo->layer              = 1;

    GameObject* player2InteractionInfo_obj = scena1->CreateGameObject(nullptr);
    SpriteComponent* player2InteractionInfo = player2InteractionInfo_obj->AddComponent<SpriteComponent>();
    player2InteractionInfo->textEnabled        = true;
    player2InteractionInfo->screenPosition     = glm::vec2(1440.0f, 640.0f);
    player2InteractionInfo->text               = "";
    player2InteractionInfo->textOutlineEnabled = true;
    player2InteractionInfo->textCentered       = true;
    player2InteractionInfo->layer              = 1;

    connectAllModels();
    LoadPlayerAnimations();

    GameObject* model1 = bed1Model->Instantiate(*scena1, nullptr, nullptr);
    model1->GetComponent<TransformComponent>()->position.x = 0.0f;
    model1->GetComponent<TransformComponent>()->position.y = 100.0f;
    model1->GetComponent<TransformComponent>()->position.z = 20.0f;

    //model1->AddComponent<RigidbodyComponent>();
    model1->AddComponent<ColliderComponent>();
    //model1->AddComponent<LightComponent>();

    //model1->GetComponent<LightComponent>()->type  = Directional;
    //model1->GetComponent<LightComponent>()->index = 0;
    //auto* light = model1->GetComponent<LightComponent>();

    //light->direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.1f));
    //light->ambient   = glm::vec3(0.2f);
    //light->diffuse   = glm::vec3(0.3f);
    //light->specular  = glm::vec3(0.9f);

    //GLuint whiteSpecular = ResourceManager::CreateTextureFromColor("white_spec", glm::vec3(1.0f)).id;
    //RenderHelper::SetSpecularTexture(model1, whiteSpecular);

    focused = true;
    updateFocus();


    renderSystem         = ecs.GetSystem<RenderSystem>();
    postProcessingSystem = ecs.GetSystem<PostProcessingSystem>();

    createFirstRoom(scena1);
    createMainRooom(scena1);
    createNuclearRooom(scena1);
    createCrematorium(scena1);
    createRentgenRoom(scena1);

    ecs.GetSystem<NavMeshSystem>()->Bake(*scena1);

    dyingModelPrefab   = std::make_unique<Prefab>("res/models/Dying.fbx");
    jumpSkeletonPrefab = std::make_unique<Prefab>("res/models/Jump.fbx");

    GameObject* dyingObj = dyingModelPrefab->Instantiate(*scena1, nullptr, nullptr);
    dyingObj->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, -50.0f, -50.0f);
    dyingObj->GetComponent<TransformComponent>()->scale    = glm::vec3(0.1f);

    AnimatorComponent* animator = dyingObj->AddComponent<AnimatorComponent>();

    // PostProcessTest
    RedModel   = std::make_unique<Prefab>("res/models/test/red_test.glb");
    GreenModel = std::make_unique<Prefab>("res/models/test/green_test.glb");
    BlueModel  = std::make_unique<Prefab>("res/models/test/blue_test.glb");

    GameObject* redObject   = RedModel->Instantiate(*scena1,   nullptr, nullptr);
    GameObject* blueObject  = BlueModel->Instantiate(*scena1,  nullptr, nullptr);
    GameObject* greenObject = GreenModel->Instantiate(*scena1, nullptr, nullptr);

    redObject->GetComponent<TransformComponent>()->position   = glm::vec3(0.0f, 30.0f,  50.0f);
    blueObject->GetComponent<TransformComponent>()->position  = glm::vec3(0.0f, 30.0f,   0.0f);
    greenObject->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 30.0f, -50.0f);

    rigidBodyCamera1->useGravity = true;
    rigidBodyCamera2->useGravity = true;

    // FMOD
    FMOD::Sound* sound = nullptr;
    ecs.GetSystem<AudioSystem>()->createSound("res/sound/door_unlock.wav", sound);

    // obracanie
    std::unordered_map<GameObject*, float> rotatingObjects;
    std::unordered_set<GameObject*> rotatingInProgress;

    auto normalizeAngle = [](float angle) -> float {
        angle = fmod(angle, 360.0f);
        if (angle < 0.0f) angle += 360.0f;
        return angle;
    };

    auto checkKibelUstawienia = [&]() {
        const float expectedAngles[6] = { 0.0f, -60.0f, -180.0f, -120.0f, -240.0f, -300.0f };

        bool allCorrect = true;
        for (int i = 0; i < 6; i++) {
            TransformComponent* transform = tablicaPapierowKibel[i]->GetComponent<TransformComponent>();
            if (transform == nullptr) { allCorrect = false; continue; }

            float current  = normalizeAngle(transform->rotation.z);
            float expected = normalizeAngle(expectedAngles[i]);

            bool correct = fabs(current - expected) < 1.0f;
            //spdlog::info("Kibel[{}] rotacja Z: {:.2f} (oczekiwana: {:.2f}) - {}",
            //    i, current, expected, correct ? "OK" : "ZLE");

            if (!correct) allCorrect = false;
        }

        if (allCorrect == true) {
            ecs.GetSystem<AudioSystem>()->playSound(sound);
        }

        can_open_door_1 = allCorrect;
    };

    // Karaluch center
    glm::vec3 nestPos = glm::vec3(0.0f, 0.5f, -80.0f);

    //I LOVE THE TASTE OF IRON
    GameObject* Kurorushi = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, nestPos, 4.0f);
    //I LOVE THE TASTE OF IRON

    for (int i = 0; i < 3; i++) {
        glm::vec3 offset = glm::vec3(
            (float)(rand() % 6) - 3.0f, 0,
            (float)(rand() % 6) - 3.0f
        );
        CreateCockroachFollower(
            *scena1, *cockroachModel, nullptr,
            Kurorushi, nestPos + offset, 4.5f); // I LOVE THE TASE OF IRON
    }

    //interfejs sprite'y
    // Crosshair P1
    GameObject* crosshair1_obj = scena1->CreateGameObject(nullptr);
    SpriteComponent* crosshair1 = crosshair1_obj->AddComponent<SpriteComponent>();
    crosshair1->sprites         = { ResourceManager::LoadTexture("crosshair.png", "res/sprites/").id };
    crosshair1->screenPosition  = glm::vec2(480.0f - 16.0f, 540.0f - 16.0f); // centrum - half size
    crosshair1->size            = glm::vec2(16.0f, 16.0f);
    crosshair1->layer           = 2; // nad napisami
    crosshair1->isVisible       = true;

    // Crosshair P2
    GameObject* crosshair2_obj = scena1->CreateGameObject(nullptr);
    SpriteComponent* crosshair2 = crosshair2_obj->AddComponent<SpriteComponent>();
    crosshair2->sprites         = { ResourceManager::LoadTexture("crosshair.png", "res/sprites/").id };
    crosshair2->screenPosition  = glm::vec2(1440.0f - 16.0f, 540.0f - 16.0f);
    crosshair2->size            = glm::vec2(16.0f, 16.0f);
    crosshair2->layer           = 2;
    crosshair2->isVisible       = true;

    const glm::vec2 CH_SIZE_NORMAL(16.0f, 16.0f);
    const glm::vec2 CH_SIZE_BIG(32.0f, 32.0f);
    const glm::vec2 CH1_CENTER(480.0f,  540.0f);
    const glm::vec2 CH2_CENTER(1440.0f, 540.0f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        updateFPS(deltaTime);

        CpuTimer cpuTimer;
        cpuTimer.start();

        bool allFilled = !puzzleSlotsMap.empty() && std::all_of(
    puzzleSlotsMap.begin(), puzzleSlotsMap.end(),
    [](const auto& pair) { return pair.second.occupant != nullptr; }
);

        for (auto& [slotGO, slot] : puzzleSlotsMap) {
            if (slot.lightObject == nullptr) continue;
            LightComponent* light = slot.lightObject->GetComponent<LightComponent>();
            if (light == nullptr) continue;

            if (!allFilled) {
                light->isOn    = false;
                light->diffuse = glm::vec3(0.0f);
                light->ambient = glm::vec3(0.0f);
                light->specular = glm::vec3(0.0f);
            } else if (slot.occupant == slot.expectedObject) {
                light->isOn     = true;
                light->diffuse  = glm::vec3(0.0f, 1.0f, 0.0f);
                light->ambient  = glm::vec3(0.0f, 0.1f, 0.0f);
                light->specular = glm::vec3(0.0f, 0.5f, 0.0f);
            } else {
                light->isOn     = true;
                light->diffuse  = glm::vec3(1.0f, 0.0f, 0.0f);
                light->ambient  = glm::vec3(0.1f, 0.0f, 0.0f);
                light->specular = glm::vec3(0.5f, 0.0f, 0.0f);
            }
        }

        UpdateDoors(deltaTime);
        UpdateCabinets(deltaTime);

        auto inputStart = std::chrono::high_resolution_clock::now();

        if (ecs.GetSystem<HID>()->is_action_just_pressed("right_click")) {
            focused = !focused;
            updateFocus();
        }
        if (ecs.GetSystem<HID>()->is_action_just_pressed("toggle_frustum_culling")) {
            renderSystem->frustumCullingEnabled = !renderSystem->frustumCullingEnabled;
           // spdlog::info("Frustum culling: {}",
           //     renderSystem->frustumCullingEnabled ? "ON" : "OFF");
        }
        if (ecs.GetSystem<HID>()->is_action_just_pressed("toggle_oclussion_culling")) {
            renderSystem->occlusionCullingEnabled = !renderSystem->occlusionCullingEnabled;
           // spdlog::info("Oclussion culling: {}",
            //    renderSystem->frustumCullingEnabled ? "ON" : "OFF");
        }

        if (ecs.GetSystem<HID>()->is_action_just_pressed("gamma_up")) {
            postProcessingSystem->set_gamma(postProcessingSystem->get_gamma() + 0.1f);
        }
        if (ecs.GetSystem<HID>()->is_action_just_pressed("gamma_down")) {
            postProcessingSystem->set_gamma(postProcessingSystem->get_gamma() - 0.1f);
        }

        if (ecs.GetSystem<HID>()->is_action_just_pressed("play_sound")) {
            ecs.GetSystem<AudioSystem>()->playSound(sound);
        }

        std::string hintText = "";

        if (player1Raycast->anyHit()) {
            RaycastHit hit = player1Raycast->closestHit();
            if (hit.hitObject != nullptr) {
                if (puzzleSlotsMap.count(hit.hitObject)) {
                    PuzzleSlot& slot = puzzleSlotsMap[hit.hitObject];
                    if (p1HeldObject != nullptr)
                        hintText = (slot.occupant == nullptr) ? "Put in" : "Slot occupied";
                    else if (slot.occupant != nullptr)
                        hintText = "Pull out";
                }
                else if (p1HeldObject != nullptr) {
                    hintText = "Drop";
                }
                else if (rotatableObjects.count(hit.hitObject))
                    hintText = "Rotate";
                else if (toiletDoorsMap.count(hit.hitObject))
                    hintText = toiletDoorsMap[hit.hitObject].isOpen ? "Close" : "Open";
                else if (cabinetsMap.count(hit.hitObject))
                    hintText = isCabinetButtonPushed ? "..." : "Open Cabinet";
                else if (majorDoors.count(hit.hitObject))
                    hintText = can_open_door_1 ? "Open" : "Unlock";
                else if (pickupObjects.count(hit.hitObject)) {
                    bool isInSlot = false;
                    for (auto& [slotGO, slot] : puzzleSlotsMap)
                        if (slot.occupant == hit.hitObject) { isInSlot = true; break; }
                    hintText = isInSlot ? "Pull out" : (hit.hitObject == p2HeldObject ? "Held by Player 2" : "Pick up");
                }
                else if (hit.hitObject->name.find("Coffin") != std::string::npos)
                    hintText = "Pull Coffin";
            }
        } else if (p1HeldObject != nullptr) {
            hintText = "Drop";
        }
        player1InteractionInfo->text = hintText;

        std::string hintText2 = "";
        if (player2Raycast->anyHit()) {
            RaycastHit hit = player2Raycast->closestHit();
            if (hit.hitObject != nullptr) {
                if (puzzleSlotsMap.count(hit.hitObject)) {
                    PuzzleSlot& slot = puzzleSlotsMap[hit.hitObject];
                    if (p2HeldObject != nullptr)
                        hintText2 = (slot.occupant == nullptr) ? "Put in" : "Slot occupied";
                    else if (slot.occupant != nullptr)
                        hintText2 = "Pull out";
                }
                else if (p2HeldObject != nullptr) {
                    hintText2 = "Drop";
                }
                else if (rotatableObjects.count(hit.hitObject))
                    hintText2 = "Rotate";
                else if (toiletDoorsMap.count(hit.hitObject))
                    hintText2 = toiletDoorsMap[hit.hitObject].isOpen ? "Close" : "Open";
                else if (cabinetsMap.count(hit.hitObject))
                    hintText2 = isCabinetButtonPushed ? "..." : "Open Cabinet";
                else if (majorDoors.count(hit.hitObject))
                    hintText2 = can_open_door_1 ? "Open" : "Unlock";
                else if (pickupObjects.count(hit.hitObject)) {
                    bool isInSlot = false;
                    for (auto& [slotGO, slot] : puzzleSlotsMap)
                        if (slot.occupant == hit.hitObject) { isInSlot = true; break; }
                    hintText2 = isInSlot ? "Pull out" : (hit.hitObject == p1HeldObject ? "Held by Player 1" : "Pick up");
                }
                else if (hit.hitObject->name.find("Coffin") != std::string::npos)
                    hintText2 = "Pull Coffin";
            }
        } else if (p2HeldObject != nullptr) {
            hintText2 = "Drop";
        }
        player2InteractionInfo->text = hintText2;

        for (auto it = rotatingObjects.begin(); it != rotatingObjects.end(); )
        {
            TransformComponent* transform = it->first->GetComponent<TransformComponent>();
            if (transform == nullptr) {
                rotatingInProgress.erase(it->first);
                it = rotatingObjects.erase(it);
                continue;
            }

            float remaining = std::abs(it->second);
            float step = 90.0f * deltaTime;
            if (step > remaining) step = remaining;
            transform->isDirty = true;

            float dir = (it->second > 0.0f) ? -1.0f : 1.0f;
            transform->rotation.z += dir * step;
            it->second = (it->second > 0.0f) ? it->second - step : it->second + step;

            if (std::abs(it->second) <= 0.0f) {
            //    spdlog::info("Rotated to: {:.2f}", transform->rotation.z);
                transform->isDirty = false;
                rotatingInProgress.erase(it->first);
                it = rotatingObjects.erase(it);
                checkKibelUstawienia();
            }
            else ++it;
        }

        HandlePlayerInteraction(ecs, "interact_p1", player1Raycast, camera1, p1HeldObject, p2HeldObject, scena1, rotatingObjects, rotatingInProgress, p1ShakeTimer, p1Animator, postacGracza.get());
        HandlePlayerInteraction(ecs, "interact_p2", player2Raycast, camera2, p2HeldObject, p1HeldObject, scena1, rotatingObjects, rotatingInProgress, p2ShakeTimer, p2Animator, postacGracza.get());

        HandleAltRotate(ecs, "alt_interact_p1", player1Raycast, rotatingObjects, rotatingInProgress);
        HandleAltRotate(ecs, "alt_interact_p2", player2Raycast, rotatingObjects, rotatingInProgress);

        // testy animacji
        if (ecs.GetSystem<HID>()->is_action_just_pressed("anim_play_dying")) {
            auto* clip = AnimationHelper::FindAnimation(dyingModelPrefab->rootModel->animations, "mixamo.com");
            if (clip) {
                AnimationHelper::Play(animator, clip, true, 1.0f);
               // spdlog::info("Odtworzono animacje umierania");
            }
        }

        if (ecs.GetSystem<HID>()->is_action_just_pressed("anim_play_jump")) {
            auto* clip = &jumpSkeletonPrefab->rootModel->animations[0];
            if (clip) {
                AnimationHelper::Play(animator, clip, true, 1.0f);
               // spdlog::info("Odtworzono animacje skoku");
            }
        }

        if (ecs.GetSystem<HID>()->is_action_pressed("anim_slow_mo")) {
            animator->playbackSpeed = 0.5f;
        }
        else if (ecs.GetSystem<HID>()->is_action_pressed("anim_fast_forward")) {
            animator->playbackSpeed = 2.0f;
        }
        else {
            animator->playbackSpeed = 1.0f;
        }

        input();

        bool p1IsMoving = false;
        bool p2IsMoving = false;

        if (focused) {
            p1IsMoving |= processCameraInput(ecs, *camCompLeft, *t0, "move_up", "move_down", "move_left", "move_right");
            p2IsMoving |= processCameraInput(ecs, *camCompRight, *t1, "move_up_2", "move_down_2", "move_left_2", "move_right_2");

            processCameraMouse(ecs, *camCompLeft, *camTransform1, *t0);

            p1IsMoving |= processCameraGamepad(ecs, *camCompLeft, *camTransform1, *t0, 0);
            p2IsMoving |= processCameraGamepad(ecs, *camCompRight,*camTransform2, *t1, 1);
        }

        bool p1IsHolding = (p1HeldObject != nullptr);
        bool p2IsHolding = (p2HeldObject != nullptr);

        PlayerAnimationHelper::UpdateAnimation(p1Animator, postacGracza.get(), p1IsMoving, p1IsHolding);
        PlayerAnimationHelper::UpdateAnimation(p2Animator, postacGracza.get(), p2IsMoving, p2IsHolding);

        if (p1ShakeTimer > 0.0f) p1ShakeTimer -= deltaTime;
        if (p2ShakeTimer > 0.0f) p2ShakeTimer -= deltaTime;

        if (p1ShakeTimer > 0.0f) {
            float t = currentFrame * 40.0f;
            float strength = 6.0f;
            player1InteractionInfo->screenPosition = p1BasePos + glm::vec2(
                sin(t)        * strength,
                sin(t * 1.3f) * strength * 0.5f
            );
        } else {
            player1InteractionInfo->screenPosition = p1BasePos;
        }

        if (p2ShakeTimer > 0.0f) {
            float t = currentFrame * 40.0f;
            float strength = 6.0f;
            player2InteractionInfo->screenPosition = p2BasePos + glm::vec2(
                sin(t + 1.0f) * strength,
                sin(t * 1.3f + 1.0f) * strength * 0.5f
            );
        } else {
            player2InteractionInfo->screenPosition = p2BasePos;
        }

        float p1Int = 0.0f, p2Int = 0.0f;

        if (player1Raycast->anyHit()) {
            auto hit = player1Raycast->closestHit();
            if (hit.hitObject &&
                (rotatableObjects.count(hit.hitObject) ||
                 toiletDoorsMap.count(hit.hitObject)   ||
                 cabinetsMap.count(hit.hitObject)       ||
                 majorDoors.count(hit.hitObject)        ||
                 pickupObjects.count(hit.hitObject)     ||
                 hit.hitObject->name.find("Coffin") != std::string::npos ||
                 (puzzleSlotsMap.count(hit.hitObject) && puzzleSlotsMap[hit.hitObject].occupant != nullptr)))
                            p1Int = 1.0f;
        }
        if (player2Raycast->anyHit()) {
            auto hit = player2Raycast->closestHit();
            if (hit.hitObject &&
                 (rotatableObjects.count(hit.hitObject) ||
                 toiletDoorsMap.count(hit.hitObject)   ||
                 cabinetsMap.count(hit.hitObject)       ||
                 majorDoors.count(hit.hitObject)        ||
                 pickupObjects.count(hit.hitObject)     ||
                 (puzzleSlotsMap.count(hit.hitObject) && puzzleSlotsMap[hit.hitObject].occupant != nullptr)))
                            p2Int = 1.0f;
        }

        float chLerpSpeed = 10.0f;
        crosshair1->size = glm::mix(crosshair1->size, p1Int > 0.5f ? CH_SIZE_BIG : CH_SIZE_NORMAL, deltaTime * chLerpSpeed);
        crosshair2->size = glm::mix(crosshair2->size, p2Int > 0.5f ? CH_SIZE_BIG : CH_SIZE_NORMAL, deltaTime * chLerpSpeed);

        crosshair1->screenPosition = CH1_CENTER - crosshair1->size * 0.5f;
        crosshair2->screenPosition = CH2_CENTER - crosshair2->size * 0.5f;

        auto inputEnd = std::chrono::high_resolution_clock::now();

        auto logicStart = std::chrono::high_resolution_clock::now();
        crematoriumPuzzle.Update(deltaTime);
        sceneManager.Update(deltaTime);
        update();
        auto logicEnd = std::chrono::high_resolution_clock::now();

        //imgui_begin();
        //imgui_render(sceneManager);
        //imgui_end();

        cpuTimer.stop();

        float cpuFrameTime = cpuTimer.getMilliseconds();
        float logicTime    = std::chrono::duration<float, std::milli>(logicEnd    - logicStart).count();
        float inputTime    = std::chrono::duration<float, std::milli>(inputEnd    - inputStart).count();

        perf.cpuFrameTime = cpuFrameTime;
        perf.logicTime    = logicTime;
        perf.inputTime    = inputTime;

        end_frame();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    //ImGui_ImplOpenGL3_Shutdown();
    //ImGui_ImplGlfw_Shutdown();
    //ImGui::DestroyContext();

    glfwDestroyWindow(window);

    groundModel.reset();
    sunModel.reset();

    ResourceManager::Clear();

    glfwTerminate();

    return 0;
}

bool init()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
      //  spdlog::error("Failed to initalize GLFW!");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MimiCry", NULL, NULL);
    if (window == NULL) {
       // spdlog::error("Failed to create GLFW Window!");
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window,       mouse_callback);
    glfwSetScrollCallback(window,          scroll_callback);

    bool err = !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    if (err) {
        //spdlog::error("Failed to initialize OpenGL loader!");
        return false;
    }
    return true;
}

/*void init_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsDark();
}*/

void compileShader()
{
    //spdlog::info("Success");
}

void input()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void update()
{
    // Update game objects' state here
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX      = xpos;
        lastY      = ypos;
        firstMouse = false;
    }

    float xoffset =  xpos - lastX;
    float yoffset =  lastY - ypos;

    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
}

/*void imgui_begin()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}*/

GameObject* selectedGameObject = nullptr;


void ShowGameObjectTree(GameObject* obj)
{
    if (!obj) return;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ((obj == selectedGameObject) ? ImGuiTreeNodeFlags_Selected : 0);

    const char* displayName = obj->name.empty() ? "GameObject" : obj->name.c_str();

    if (!obj->HasChildren())
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool opened = ImGui::TreeNodeEx((void*)obj, flags, "%s", displayName);

    /*if (ImGui::IsItemClicked())
        selectedGameObject = obj;*/

    if (opened) {
        for (GameObject* child : obj->GetChildren())
            ShowGameObjectTree(child);
        //ImGui::TreePop();
    }
}

/*void ShowTransformEditor(TransformComponent& transform)
{
    glm::vec3 pos   = TransformHelper::getLocalPosition(transform);
    glm::vec3 rot   = TransformHelper::getLocalRotation(transform);
    glm::vec3 scale = TransformHelper::getLocalScale(transform);

    if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
        TransformHelper::setLocalPosition(transform, pos);

    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
        TransformHelper::setLocalRotation(transform, rot);

    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.01f))
        TransformHelper::setLocalScale(transform, scale);
}

void ShowRigidbodyEditor(RigidbodyComponent& rb)
{
    ImGui::Text("Rigidbody");

    ImGui::DragFloat("Mass", &rb.mass, 0.01f, 0.0f, 1000.0f);
    ImGui::DragFloat("Bounce", &rb.bounce, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Angular Damping", &rb.angularDamping, 0.01f, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Checkbox("Use Gravity", &rb.useGravity);
    ImGui::Checkbox("Is Static", &rb.isStatic);

    ImGui::Separator();
    ImGui::DragFloat3("Velocity", &rb.velocity.x, 0.01f);
    ImGui::DragFloat3("Acceleration", &rb.acceleration.x, 0.01f);
    ImGui::DragFloat3("Angular Velocity", &rb.angularVelocity.x, 0.01f);
    ImGui::DragFloat3("Torque", &rb.torque.x, 0.01f);

    ImGui::Separator();
    ImGui::BeginDisabled();
    ImGui::DragFloat3("Physics Position", &rb.physicsPosition.x, 0.01f);
    ImGui::DragFloat3("Previous Position", &rb.previousPosition.x, 0.01f);
    ImGui::EndDisabled();

    if (ImGui::Button("Reset Velocity")) {
        rb.velocity = glm::vec3(0.0f);
        rb.angularVelocity = glm::vec3(0.0f);
        rb.torque = glm::vec3(0.0f);
        rb.acceleration = glm::vec3(0.0f);
    }
}

void ShowColliderEditor(ColliderComponent& col)
{
    ImGui::Text("Collider");
    ImGui::DragFloat3("Offset", &col.offset.x, 0.01f);
    ImGui::DragFloat3("HalfSize", &col.halfSize.x, 0.01f, 0.0f);
    ImGui::Checkbox("Is Trigger", &col.isTrigger);
    ImGui::Checkbox("Affects NavMesh", &col.affectsNavMesh);
    ImGui::Checkbox("Is Walkable", &col.isWalkable);
}*/

/*void ShowLightEditor(LightComponent& light)
{
    ImGui::Text("Light");
    ImGui::Checkbox("Enabled", &light.isOn);

    const char* lightTypes[] = { "Directional", "Point", "Spot" };
    int currentType = static_cast<int>(light.type);
    if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
        light.type = static_cast<LightType>(currentType);

    ImGui::Separator();
    ImGui::ColorEdit3("Ambient",  &light.ambient.x);
    ImGui::ColorEdit3("Diffuse",  &light.diffuse.x);
    ImGui::ColorEdit3("Specular", &light.specular.x);
    ImGui::DragFloat("Intensity", &light.intensity, 0.001f, 0.0f, 1000.0f);
    ImGui::Separator();

    if (light.type == Point || light.type == Spot) {
        ImGui::Text("Attenuation");
        ImGui::DragFloat("Constant",  &light.constant,  0.001f, 0.0f, 10.0f);
        ImGui::DragFloat("Linear",    &light.linear,    0.001f, 0.0f, 10.0f);
        ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 10.0f);
        ImGui::DragFloat("Range", &light.range, 0.001f, 0.0f, 1000.0f);
    }

    if (light.type == Spot) {
        ImGui::Separator();
        ImGui::Text("Spotlight");

        float innerAngle = glm::degrees(glm::acos(light.cutOff));
        float outerAngle = glm::degrees(glm::acos(light.outerCutOff));

        if (ImGui::DragFloat("Inner Cutoff", &innerAngle, 0.1f, 0.0f, 90.0f))
            light.cutOff = glm::cos(glm::radians(innerAngle));

        if (ImGui::DragFloat("Outer Cutoff", &outerAngle, 0.1f, 0.0f, 90.0f))
            light.outerCutOff = glm::cos(glm::radians(outerAngle));
    }
}
*/
std::string OpenFileDialog()
{
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn   = {};
    ofn.lStructSize     = sizeof(OPENFILENAMEA);
    ofn.lpstrFile       = filename;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrInitialDir = "res";
    ofn.lpstrFilter     =
        "Model Files\0*.obj;*.fbx;*.glb;*.gltf\0"
        "All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        std::filesystem::path fullPath    = filename;
        std::filesystem::path projectRoot = std::filesystem::absolute("../../");
        std::filesystem::path relative    = std::filesystem::relative(fullPath, projectRoot);
        std::string result                = relative.string();
        std::replace(result.begin(), result.end(), '\\', '/');
        return result;
    }
    return "";
}

static std::unordered_map<std::string, Prefab> prefabs;

/*
void imgui_render(SceneManager& sceneManager)
{
  if (show_demo_window) { }

    ImGui::Begin("Hello, world!");

    if (ImGui::Button(wireframeMode ? "Switch to Fill Mode" : "Switch to Wireframe")) {
        wireframeMode = !wireframeMode;
        glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
    }

    ImGui::Separator();
    ImGui::Text("Ambient");
    ImGui::DragFloat("Ambient strength", &renderSystem->ambientStrength, 0.000001f, 0.0f, 1.0f, "%.6f");
    ImGui::Separator();
    ImGui::Text("Hierarchy");
    ShowGameObjectTree(sceneManager.GetActiveScene()->GetRoot());

    if (selectedGameObject) {
        ImGui::Separator();
        ImGui::Text("Selected Entity: %s", selectedGameObject->name.c_str());
        ShowTransformEditor(*selectedGameObject->GetComponent<TransformComponent>());

        RigidbodyComponent* rb = selectedGameObject->GetComponent<RigidbodyComponent>();
        if (rb != nullptr)
            ShowRigidbodyEditor(*rb);

        ColliderComponent* col = selectedGameObject->GetComponent<ColliderComponent>();
        if (col != nullptr)
            ShowColliderEditor(*col);

        LightComponent* light = selectedGameObject->GetComponent<LightComponent>();
        if (light != nullptr)
            ShowLightEditor(*light);
    }

    if (ImGui::Button("Zapisz"))
        sceneManager.Save();

    ImGui::Separator();

    Scene& scene = *sceneManager.GetActiveScene();

    for (auto& [name, weakModel] : ResourceManager::Models) {
        ImGui::PushID(name.c_str());
        std::shared_ptr<Model> model = weakModel;
        ImGui::Text("%s", name.c_str());
        ImGui::SameLine();

        if (!model) {
            ImGui::TextDisabled("[loading]");
        }
        else {
            if (!prefabs.contains(name))
                prefabs.emplace(name, Prefab(model));

            if (ImGui::Button("Instantiate")) {
                Prefab& prefab = prefabs.at(name);
                GameObject* obj = prefab.Instantiate(scene, nullptr, nullptr);
                if (obj) obj->name = name;
            }
        }
        ImGui::PopID();
    }

    ImGui::End();



    ImGui::Begin("Loaded Models");

    if (ImGui::Button("Load Model")) {
        std::string path = OpenFileDialog();
        if (!path.empty())
            ResourceManager::LoadModel(path);
    }

    if (ImGui::Button("Load asset")) {
        std::string path = "assets.yaml";
        ResourceManager::LoadAssets(path);
    }

    if (ImGui::Button("Zapisz asset"))
        ResourceManager::SaveAsset();

    ImGui::Separator();

    for (const auto& [name, weakModel] : ResourceManager::Models)
        ImGui::Text("%s", name.c_str());

    ImGui::End();

    if (ImGui::Begin("Debug hizTexture")) {
        auto system = sceneManager.GetActiveScene()->GetECS().GetSystem<RenderSystem>();
        static int debugMip = 0;
        ImGui::SliderInt("Mip", &debugMip, 0, 10);
        int i = 0;
        for (auto& [cam, hiz] : system->cameraHiZ) {
            ImGui::Text("Kamera %d", i++);
            if (hiz.hizTexture != 0)
                system->ShowR32FTextureImGui(hiz.hizTexture, debugMip);
        }
    }
    ImGui::End();

    // Osobno — cały ImGui
    //static int debugMip = 0;
    //ImGui::Begin("HiZ Debug Controls");
    //auto* renderer = system->drivenManager.GetRenderer(0);
    //ImGui::SliderInt("Mip", &debugMip, 0, renderer->hizMipLevels - 1);
    //ImGui::End();

    //// Pobierz renderer żeby wywołać debug
    //if (renderer) renderer->DebugShowHiZ(debugMip);

    ImGui::Begin("Performance");

    ImGui::Text("FPS: %.1f",        ImGui::GetIO().Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    static bool vsyncEnabled = true;
    if (ImGui::Checkbox("VSync", &vsyncEnabled))
    {
        glfwSwapInterval(vsyncEnabled ? 1 : 0);
    }

    if (ImGui::CollapsingHeader("CPU")) {
        ImGui::Text("Total CPU: %.3f ms", perf.cpuFrameTime);
        ImGui::Text("Input:     %.3f ms", perf.inputTime);
        ImGui::Text("Logic:     %.3f ms", perf.logicTime);
        ImGui::Text("Culling:   %.3f ms", renderSystem->stats.cullingTimeMs);
        ImGui::Text("Draw prep: %.3f ms", renderSystem->stats.drawSubmitTimeMs);
    }
    if (ImGui::CollapsingHeader("GPU")) {
        ImGui::Text("GPU Frame: %.3f ms", renderSystem->gpuQuery.getLastResult());
    }
    if (ImGui::CollapsingHeader("Render Stats")) {
        ImGui::Text("Draw calls:    %d", renderSystem->stats.drawCalls);
        ImGui::Text("Objects:       %d", renderSystem->stats.renderedObjects);
        ImGui::Text("Triangles:     %d", renderSystem->stats.triangles);
        ImGui::Text("State changes: %d", renderSystem->stats.stateChanges);
    }
    if (ImGui::CollapsingHeader("Culling")) {
        ImGui::Checkbox("Frustum culling",   &renderSystem->frustumCullingEnabled);
        ImGui::Checkbox("Occlusion culling", &renderSystem->occlusionCullingEnabled);
        ImGui::Text("Frustum culled:   %d", renderSystem->stats.frustumCulledSet.size());
        ImGui::Text("Occlusion culled: %d", renderSystem->stats.occlusionCulledSet.size());
    }
    ImGui::PlotLines("Frame time", frameTimes, MAX_SAMPLES, index,
        nullptr, 0.0f, 1.0f, ImVec2(0, 60));

    ImGui::End();

    ImGui::Begin("Puzzle Debug");

    if (puzzleSlotsMap.empty()) {
        ImGui::TextColored(ImVec4(1,1,0,1), "Brak slotów puzzle");
    } else {
        int i = 0;
        for (auto& [slotGO, slot] : puzzleSlotsMap) {
            ImGui::PushID(i++);

            std::string slotName = slotGO ? slotGO->name : "???";
            std::string expectedName = slot.expectedObject ? slot.expectedObject->name : "???";
            std::string occupantName = slot.occupant       ? slot.occupant->name       : "(pusty)";

            bool isEmpty   = (slot.occupant == nullptr);
            bool isCorrect = (!isEmpty && slot.occupant == slot.expectedObject);

            // Kolor: zielony = dobry, czerwony = zły, szary = pusty
            ImVec4 color = isEmpty
                ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f)   // szary
                : isCorrect
                    ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) // zielony
                    : ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // czerwony

            const char* status = isEmpty ? "[PUSTY]" : isCorrect ? "[OK]" : "[ZLE]";

            ImGui::TextColored(color, "%s  Slot: %-20s  Oczekiwany: %-10s  Aktualny: %-10s",
                status,
                slotName.c_str(),
                expectedName.c_str(),
                occupantName.c_str()
            );

            ImGui::PopID();
        }

        ImGui::Separator();

        // Podsumowanie
        int correct = 0, filled = 0;
        for (auto& [slotGO, slot] : puzzleSlotsMap) {
            if (slot.occupant != nullptr) filled++;
            if (slot.occupant != nullptr && slot.occupant == slot.expectedObject) correct++;
        }
        int total = (int)puzzleSlotsMap.size();

        ImGui::Text("Wypełnione: %d / %d", filled, total);
        ImGui::Text("Poprawne:   %d / %d", correct, total);

        if (correct == total)
            ImGui::TextColored(ImVec4(0,1,0,1), ">> PUZZLE ROZWIAZANY! <<");
    }

    ImGui::End();
}

void imgui_end()
{
    ImGui::Render();
    int display_w, display_h;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
} */

void end_frame()
{
    glfwPollEvents();
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}


void addAllSystems(ECS& ecs) {
    ecs.AddSystem<TransformSystem>(ecs);
    ecs.AddSystem<PhysicsSystem>(ecs);
    ecs.AddSystem<AnimationSystem>(ecs);
    ecs.AddSystem<RenderSystem>(ecs, window);
    ecs.AddSystem<HID>(ecs, window);
    ecs.AddSystem<PostProcessingSystem>(ecs, window);
    ecs.AddSystem<SpriteSystem>(ecs, window);
    ecs.AddSystem<RaycastSystem>(ecs);
    ecs.AddSystem<NavMeshSystem>(ecs);
    ecs.AddSystem<NavPathSystem>(ecs);
    ecs.AddSystem<AudioSystem>(ecs);
    ecs.AddSystem<NpcSystem>(ecs);
}

void LoadPlayerAnimations() {
    //spdlog::info("Mapowanie animacji z pojedynczego pliku .glb gracza...");

    if (!postacGracza || !postacGracza->rootModel) return;

    std::vector<AnimationClip> rawAnimations = postacGracza->rootModel->animations;
    postacGracza->rootModel->animations.clear();
    postacGracza->rootModel->animations.resize(7);

    auto findAndMap = [&](const std::string& exactName, int targetIdx, const std::string& fallbackName = "") {
        for (auto& anim : rawAnimations) {
            if (anim.name == exactName) {
                postacGracza->rootModel->animations[targetIdx] = anim;
                //spdlog::info("Zmapowano animację '{}' pod indeks [{}]", anim.name, targetIdx);
                return true;
            }
        }

        for (auto& anim : rawAnimations) {
            if (anim.name.find(exactName) != std::string::npos) {
                postacGracza->rootModel->animations[targetIdx] = anim;
                //spdlog::info("Zmapowano (częściowo) animację '{}' pod indeks [{}]", anim.name, targetIdx);
                return true;
            }
        }

        if (!fallbackName.empty()) {
            for (auto& anim : rawAnimations) {
                if (anim.name.find(fallbackName) != std::string::npos) {
                    postacGracza->rootModel->animations[targetIdx] = anim;
                    //spdlog::warn("Brak '{}', użyto zamiennika '{}' dla indeksu [{}]", exactName, anim.name, targetIdx);
                    return true;
                }
            }
        }
        return false;
        };

    findAndMap("idle", 0);
    findAndMap("walk", 1, "idle");
    findAndMap("pick", 2, "idle");

    findAndMap("idle", 3, "idle");

    findAndMap("idle_pick", 4, "idle");
    findAndMap("walk_pick", 5, "walk");
    findAndMap("interaction", 6, "idle");
}

void connectAllModels() {
    bed1Model        = std::make_unique<Prefab>("res/models/bed.glb");
    bed2Model        = std::make_unique<Prefab>("res/models/bed2.glb");
    bed3Model        = std::make_unique<Prefab>("res/models/bed3.glb");
    placeholderModel = std::make_unique<Prefab>("res/models/placeholder.glb");
    sinkModel        = std::make_unique<Prefab>("res/models/sink_2.glb");
    toiletModel      = std::make_unique<Prefab>("res/models/toilet_f.glb");
    doorsToiletModel = std::make_unique<Prefab>("res/models/doors_toliet_tex6.glb");
    toiletPaperRedModel = std::make_unique<Prefab>("res/models/papier_czerwony.glb");
    toiletPaperGreenModel = std::make_unique<Prefab>("res/models/papier_zielony.glb");
    toiletPaperBlueModel = std::make_unique<Prefab>("res/models/papier_niebieski.glb");
    mirrorModel1     = std::make_unique<Prefab>("res/models/glass1_v2.glb");
    mirrorModel2     = std::make_unique<Prefab>("res/models/lustro_puste.glb");
    mirrorModel3     = std::make_unique<Prefab>("res/models/glass2_v2.glb");
    mirrorModel4     = std::make_unique<Prefab>("res/models/glass3_v2.glb");
    washroomExit     = std::make_unique<Prefab>("res/models/door_other_2.glb");
    urinModel        = std::make_unique<Prefab>("res/models/uniral_v2.glb");
    NormalDoor       = std::make_unique<Prefab>("res/models/doors.glb");
    szafkaModel      = std::make_unique<Prefab>("res/models/szafka_pop_main.glb");
    ruraModel        = std::make_unique<Prefab>("res/models/placeholder_rura_wysuwana.glb");
    panelModel       = std::make_unique<Prefab>("res/models/Panel_5x5.glb");
    puzel1       = std::make_unique<Prefab>("res/models/Puzel1.glb");
    puzel2       = std::make_unique<Prefab>("res/models/Puzel2.glb");
    puzel3       = std::make_unique<Prefab>("res/models/Puzel3.glb");
    puzel4       = std::make_unique<Prefab>("res/models/Puzel4.glb");
    puzel5       = std::make_unique<Prefab>("res/models/Puzel5.glb");
    puzel6       = std::make_unique<Prefab>("res/models/Puzel6.glb");

    bossCapsuleModel       = std::make_unique<Prefab>("res/models/boss_capsule.glb");
    kredensModel       = std::make_unique<Prefab>("res/models/kredens.glb");
    eksp1Model = std::make_unique<Prefab>("res/models/eksp1.glb");
    eksp2Model = std::make_unique<Prefab>("res/models/eksp2.glb");
    fiolka1Model = std::make_unique<Prefab>("res/models/fiolka1.glb");
    fiolka2Model = std::make_unique<Prefab>("res/models/fiolka2.glb");
    ksiazkaModel = std::make_unique<Prefab>("res/models/ksiazka.glb");
    probowka7Model = std::make_unique<Prefab>("res/models/probowka7.glb");
    probowka6Model = std::make_unique<Prefab>("res/models/probowka6.glb");
    probowka5Model = std::make_unique<Prefab>("res/models/probowka5.glb");
    probowkaArka_1_Model = std::make_unique<Prefab>("res/models/probowka_1.glb");
    probowka3Model = std::make_unique<Prefab>("res/models/probowka3.glb");
    probowka4Model = std::make_unique<Prefab>("res/models/probowka4.glb");
    labOla1Model = std::make_unique<Prefab>("res/models/lab_ola_1.glb");
    probowka2Model = std::make_unique<Prefab>("res/models/probowka_2.glb");
    folderModel = std::make_unique<Prefab>("res/models/folder.glb");
    papersModel = std::make_unique<Prefab>("res/models/papers.glb");
    cupModel = std::make_unique<Prefab>("res/models/cup.glb");
    corkBoardModel = std::make_unique<Prefab>("res/models/cork_board.glb");
    clockModel = std::make_unique<Prefab>("res/models/clock.glb");
    computer_pbrModel = std::make_unique<Prefab>("res/models/computer_no_alpha.glb");
    laboratoryStuff1Model = std::make_unique<Prefab>("res/models/laboratory_stuff_1.glb");
    laboratoryStuff2Model = std::make_unique<Prefab>("res/models/laboratory_stuff_2.glb");
    laboratoryStuff3Model = std::make_unique<Prefab>("res/models/laboratory_stuff_3.glb");

    zielonaTablica = std::make_unique<Prefab>("res/models/ZielonaTablica.glb");
    czerwonaTablica = std::make_unique<Prefab>("res/models/CzerwonaTablica.glb");
    Rentgen = std::make_unique<Prefab>("res/models/Rentgen.glb");

    cockroachModel   = std::make_unique<Prefab>("res/models/cockroach.glb");

    bossModel = std::make_unique<Prefab>("res/models/demon_animations_with_textures.glb");
    bossCapsuleModel = std::make_unique<Prefab>("res/models/boss_capsule.glb");
}

void createFirstRoom(Scene* scena1) {
    floorModel = std::make_unique<Prefab>("res/models/number_floor.glb");
    wallModel  = std::make_unique<Prefab>("res/models/wall.glb");
    wallModel2 = std::make_unique<Prefab>("res/models/wall2.glb");
    wallModel3 = std::make_unique<Prefab>("res/models/wall3.glb");

    // Podloga i sufit
    GameObject* floor = CreateStaticObject(scena1, floorModel.get(), nullptr,
    "PodlogawLazience", glm::vec3(25, 0, -60), glm::vec3(50, 1, 50));
    floor->GetComponent<ColliderComponent>()->isWalkable = true;
    CreateStaticObject(scena1, floorModel.get(), nullptr, "SufitWKiblu",       glm::vec3(0, 20, 0),  glm::vec3(100, 1, 100), glm::vec3(0), glm::vec3(100, 1, 100));

    // Sciany
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "ScianaTylnaKibel",           glm::vec3(0, 0, -10),    glm::vec3(50, 50, 1),  glm::vec3(0), glm::vec3(50, 50, 1));
    CreateStaticObject(scena1, wallModel2.get(), nullptr, "ScianaKiblowa",              glm::vec3(50, 0, 0),     glm::vec3(100, 50, 1), glm::vec3(0,90,0), glm::vec3(1, 50, 100));
    CreateStaticObject(scena1, wallModel2.get(), nullptr, "ScianaSinkowa",              glm::vec3(-25, 0, 0),    glm::vec3(100, 50, 1), glm::vec3(0,90,0), glm::vec3(1, 50, 100));
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "ScianaDrzwiDoMainRoomPrawa", glm::vec3(110, 0, -100), glm::vec3(100, 50, 1), glm::vec3(0), glm::vec3(100,50,1), true);
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "ScianaDrzwiDoMainRoomLewa",  glm::vec3(-110, 0, -100),glm::vec3(100, 50, 1), glm::vec3(0), glm::vec3(100,50,1), true);
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "GoraPrzejscieDoMainRoom",    glm::vec3(0, 70, -100),  glm::vec3(100, 50, 1), glm::vec3(0), glm::vec3(100,50,1), true);

    // Kibel
    GameObject* tablicaKibli[8];
    for (int i = 0; i < 8; i++) {
        if (i !=2 && i != 3) {
            tablicaKibli[i] = toiletModel->Instantiate(*scena1, nullptr, nullptr);
            tablicaKibli[i]->name = "Kibel" + std::to_string(i);
            tablicaKibli[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
            tablicaKibli[i]->AddComponent<ColliderComponent>();
            tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 2.5, 4, 2.5 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->offset       = glm::vec3{ 0, 4, 0 };
            tablicaKibli[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 45, 0.5f, -25 + (-10 * i) };
            tablicaKibli[i]->GetComponent<TransformComponent>()->rotation    = glm::vec3{ 0, 90, 0 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
            tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
        }

        if (i == 2 || i == 3) {
            tablicaKibli[i] = urinModel->Instantiate(*scena1, nullptr, nullptr);
            tablicaKibli[i]->name = "Kibel" + std::to_string(i);
            tablicaKibli[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 12, 12, 12 };
            tablicaKibli[i]->AddComponent<ColliderComponent>();
            tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 2.5, 4, 2.5 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->offset       = glm::vec3{ 0, 4, 0 };
            tablicaKibli[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 47.6, 2.0f, -25 + (-10 * i) };
            tablicaKibli[i]->GetComponent<TransformComponent>()->rotation    = glm::vec3{ 0, 270, 0 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
            tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
        }
    }

    // Zaslony
    GameObject* tablicaZaslon[9];
    for (int i = 0; i < 9; i++) {

        tablicaZaslon[i] = wallModel3->Instantiate(*scena1, nullptr, nullptr);
        tablicaZaslon[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 0.3, 30, 20 };
        tablicaZaslon[i]->name = "Zaslona" + std::to_string(i);
        tablicaZaslon[i]->AddComponent<ColliderComponent>();
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 20, 15, 0.3 };
        tablicaZaslon[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 50, 0, -20 + (-10 * i) };
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;

        if (i==3) {
            tablicaZaslon[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 5000, 0, -20 + (-10 * i) };
        }
    }

    // Drzwi do kibla
    for (int i = 0; i < 8; i++) {
        if (i != 2 && i != 3) {
            glm::vec3 doorPos      = glm::vec3{ 30.0f, 6.0f, -24.65f + (-10.0f * i) };
            glm::vec3 doorScale    = glm::vec3{ 11.0f, 10.0f, 16.0f };
            glm::vec3 pivotOffset  = glm::vec3(0.2f, 0.0f, 3.8f);
            glm::vec3 colliderSize = glm::vec3{ 0.9f, 10.0f, 4.5f };

            GameObject* hinge = CreateInteractableDoor(
                scena1, doorsToiletModel.get(), nullptr,
                "ToiletDoor_" + std::to_string(i),
                doorPos, doorScale, pivotOffset, colliderSize, 90.0f
            );
            unlockedDoors.insert(hinge);
        }
    }

    // Papier toaletowy
    for (int i = 0; i < 6; i++) {
        if (i == 0 || i == 3)
            tablicaPapierowKibel[i] = toiletPaperGreenModel->Instantiate(*scena1, nullptr, nullptr);
        if (i == 1 || i == 5)
            tablicaPapierowKibel[i] = toiletPaperRedModel->Instantiate(*scena1, nullptr, nullptr);
        if (i == 2 || i == 4)
            tablicaPapierowKibel[i] = toiletPaperBlueModel->Instantiate(*scena1, nullptr, nullptr);

        tablicaKibli[i]->name = "PapierKibel" + std::to_string(i);
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaPapierowKibel[i]->AddComponent<ColliderComponent>();
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 35, 5.0, -40.7 + (-10 * i) };
        rotatableObjects.insert(tablicaPapierowKibel[i]);
        if (i < 2) {
            tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 35, 5.0, -40.7 + (-10 * i) + 20 };
        }
    }

    // Zlewy - pozycja X z MainRoomIPoprawkiModeli (-20.5)
    GameObject* tablicaSink[8];
    for (int i = 0; i < 8; i++) {
        tablicaSink[i] = sinkModel->Instantiate(*scena1, nullptr, nullptr);
        tablicaSink[i]->name = "Sink" + std::to_string(i);
        tablicaSink[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
        tablicaSink[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaSink[i]->GetComponent<TransformComponent>()->position = glm::vec3{ -20.5, 6.0, -20 + (-10 * i) };
        tablicaSink[i]->AddComponent<ColliderComponent>();
    }

    /*
    // Urinary - 7 sztuk, uklad z mastera (poziomy, skala 12,12,12, obrot 180)
    GameObject* tablicaurin[7];
    for (int i = 0; i < 7; i++) {
        tablicaurin[i] = urinModel->Instantiate(*scena1, nullptr, nullptr);
        tablicaurin[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 12, 12, 12 };
        tablicaurin[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 180, 0 };
        tablicaurin[i]->AddComponent<RigidbodyComponent>();
        tablicaurin[i]->AddComponent<ColliderComponent>();
        tablicaurin[i]->GetComponent<RigidbodyComponent>()->useGravity = false;
        tablicaurin[i]->GetComponent<RigidbodyComponent>()->isStatic   = true;
        tablicaurin[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 3, 20, 3 };
        tablicaurin[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ -17.5 + (10 * i), 2.0, -12.5 };
    }*/

    // Lustra 1-3
    GameObject* lustro1 = mirrorModel1->Instantiate(*scena1, nullptr, nullptr);
    lustro1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro1->AddComponent<ColliderComponent>();
    lustro1->GetComponent<TransformComponent>()->position   = glm::vec3{ -23.5, 12.0, -25 + (-20 * 0) };

    GameObject* lustro2 = mirrorModel2->Instantiate(*scena1, nullptr, nullptr);
    lustro2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro2->AddComponent<ColliderComponent>();
    lustro2->GetComponent<TransformComponent>()->position   = glm::vec3{ -23.5, 12.0, -25 + (-20 * 1) };

    GameObject* lustro3 = mirrorModel3->Instantiate(*scena1, nullptr, nullptr);
    lustro3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro3->AddComponent<ColliderComponent>();
    lustro3->GetComponent<TransformComponent>()->position   = glm::vec3{ -23.5, 12.0, -25 + (-20 * 2) };

    // Lustro 4 - dodane z mirrorModel4 (lustro_puste.glb)
    GameObject* lustro4 = mirrorModel4->Instantiate(*scena1, nullptr, nullptr);
    lustro4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro4->AddComponent<ColliderComponent>();
    lustro4->GetComponent<TransformComponent>()->position   = glm::vec3{ -23.5, 12.0, -25 + (-20 * 3) };

    // Drzwi wyjsciowe z lazienki (washroomExit)
    GameObject* tablicaDrzwi[2];
    for (int i = 0; i < 2; i++) {
        tablicaDrzwi[i] = washroomExit->Instantiate(*scena1, nullptr, nullptr);
        tablicaDrzwi[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 11, 10 };
        tablicaDrzwi[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 180 * i, 0 };
        tablicaDrzwi[i]->AddComponent<ColliderComponent>();
        tablicaDrzwi[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 5, 22, 1 };
        tablicaDrzwi[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ -5 + (10 * i), 0.0, -100 };
        majorDoors.insert(tablicaDrzwi[i]);
        tablicaDrzwi[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
        tablicaDrzwi[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    }

    // Kubek - kolider domyslny (bez jawnego halfSize)
    GameObject* cup = cupModel->Instantiate(*scena1, nullptr, nullptr);
    cup->name = "Magiczny_Kubek";

    TransformComponent* cupTr = cup->GetComponent<TransformComponent>();
    cupTr->position = glm::vec3{ 20.0f, 5.0f, -30.0f };
    cupTr->scale    = glm::vec3{ 2, 2, 2 };

    RigidbodyComponent* cupRb = cup->AddComponent<RigidbodyComponent>();
    cupRb->useGravity = true;
    cupRb->isStatic   = false;

    cup->AddComponent<ColliderComponent>();

    pickupObjects.insert(cup);
}

void createMainRooom(Scene* scena) {
    // Podloga i sufit
    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaMainRoom", glm::vec3(0, 0, -158),  glm::vec3(60, 1, 60));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitMainRoom",   glm::vec3(0, 25, -200), glm::vec3(100, 1, 100), std::nullopt, glm::vec3(100, 1, 100));

    // Sciany
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaDoRentgenaPrawa",        glm::vec3(35, 0, -218),   glm::vec3(25, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaDoRentgenaLewa",         glm::vec3(-30, 0, -218),  glm::vec3(30, 50, 1));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaPrawaDoKrematorium",     glm::vec3(60, 0, -112),   glm::vec3(12, 50, 1), std::nullopt, glm::vec3(1, 50, 12));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaLewaDoKrematorium",      glm::vec3(60, 0, -177.2),   glm::vec3(41.79, 50, 1), std::nullopt, glm::vec3(1, 50, 41.79));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaPrawaWRentgenie",      glm::vec3(60, 0, -260.350),   glm::vec3(41.79, 50, 1), std::nullopt, glm::vec3(1, 50, 41.79));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaPrawaDoATOMU",           glm::vec3(-60, 0, -130),  glm::vec3(30, 50, 1), std::nullopt, glm::vec3(1, 50, 30));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaLewaDoATOMU",            glm::vec3(-60, 0, -235),  glm::vec3(65, 50, 1), std::nullopt, glm::vec3(1, 50, 65));

    // Gora przejscia
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoRentgena",              glm::vec3(0, 70, -218),   glm::vec3(100, 50, 1));
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoKrematorium",           glm::vec3(60, 70, -218),  glm::vec3(100, 50, 1), glm::vec3(0, 90, 0));
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoREAKTORAATOMOWEGO",     glm::vec3(-60, 70, -218), glm::vec3(100, 50, 1), glm::vec3(0, 90, 0));

    glm::vec3 scaleDoors = glm::vec3(2.25, 2.2, 1);

    GameObject* hingeKrematorium = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoKrematorium",
        glm::vec3(59.850f, 3.000f, -131.060f), glm::vec3(2.5, 2.2, 1),
        glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.8f, 20.0f, 5.0f), -90.0f, 90.0f
    );
    toiletDoorsMap[hingeKrematorium].canBeClicked = false;
    mainRoomDoors.push_back(hingeKrematorium);

    GameObject* hingeRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoRentgen",
        glm::vec3(5.440f, 2.750f, -217.800f), scaleDoors,
        glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(5.0f, 20.0f, 0.8f), 90.0f, 0.0f
    );
    toiletDoorsMap[hingeRentgen].canBeClicked = false;
    mainRoomDoors.push_back(hingeRentgen);

    GameObject* hingeATOM = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoATOMU",
        glm::vec3(-60.440f, 2.750f, -165.180f), scaleDoors,
        glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.8f, 20.0f, 5.0f), -90.0f, 90.0f
    );
    toiletDoorsMap[hingeATOM].canBeClicked = false;
    mainRoomDoors.push_back(hingeATOM);

    GameObject* szafkaObj = szafkaModel->Instantiate(*scena, nullptr, nullptr);
    szafkaObj->name = "Szafka";

    TransformComponent* szafkaTr = szafkaObj->GetComponent<TransformComponent>();
    szafkaTr->position = glm::vec3{ 27.0f, 9.0f, -216.490f };
    szafkaTr->scale    = glm::vec3{ 8.0f, 8.0f, 8.0f };
    szafkaTr->rotation = glm::vec3{ 0.0f, -90.0f, 0.0f };

    ColliderComponent* szafkaCol = szafkaObj->AddComponent<ColliderComponent>();
    szafkaCol->affectsNavMesh = true;
    szafkaCol->halfSize        = glm::vec3{ 10.0f, 8.0f, 3.0f };
    szafkaCol->offset          = glm::vec3{ 2.0f, 6.0f, 0.0f };

    CabinetState cabState;
    szafkaObj->TraverseChildren([&](GameObject* go) {
        if (go->name == "Left_Door")  cabState.leftDoor  = go;
        if (go->name == "Right_Door") cabState.rightDoor = go;
        if (go->name == "Guzik")      cabState.button    = go;
    });

    GameObject * bossCapsule = bossCapsuleModel->Instantiate(*scena, nullptr, nullptr);
    bossCapsule->name = "BossCapsule";
    bossCapsule->AddComponent<ColliderComponent>();
    bossCapsule->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    bossCapsule->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    bossCapsule->GetComponent<TransformComponent>()->position = glm::vec3{51.58 ,5, -209.64f };
    bossCapsule->GetComponent<ColliderComponent>()->halfSize    = glm::vec3{ 5.34, 5.34f, 5.34f };

    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens1",     glm::vec3(56, 4.8, -176), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0), glm::vec3(2, 5, 13));
    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens2",     glm::vec3(56, 4.8, -152), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0), glm::vec3(2, 5, 13));
    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens3",     glm::vec3(20, 4.8, -176), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0), glm::vec3(2, 5, 13));
    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens4",     glm::vec3(20, 4.8, -152), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0), glm::vec3(2, 5, 13));
    GameObject * kredens5 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens5",     glm::vec3(27.7, 4.8, -177), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0), glm::vec3(4, 5, 13));
    GameObject * kredens6 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens6",     glm::vec3(27.7, 4.8, -153), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0), glm::vec3(4, 5, 13));
    kredens5->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };
    kredens6->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };

    GameObject * eksp1 = eksp1Model->Instantiate(*scena, nullptr, nullptr);
    eksp1->name = "eksp1";
    eksp1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    eksp1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -180.0f, 0.0f };
    eksp1->GetComponent<TransformComponent>()->position = glm::vec3{ 57.170 ,9.510, -165.340 };

    GameObject * eksp2 = eksp2Model->Instantiate(*scena, nullptr, nullptr);
    eksp2->name = "eksp2";
    eksp2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    eksp2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -111.000f, 0.0f };
    eksp2->GetComponent<TransformComponent>()->position = glm::vec3{ 57.250 ,9.000, -178.730f };

    GameObject * fiolka1 = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1->name = "fiolka1";
    fiolka1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka1->GetComponent<TransformComponent>()->position = glm::vec3{ 20 ,9, -144.64f };
    GameObject * fiolka1b = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1b->name = "fiolka1b";
    fiolka1b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -300.0f, 0.0f };
    fiolka1b->GetComponent<TransformComponent>()->position = glm::vec3{ 22 ,9, -146.64f };
    GameObject * fiolka1c = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1c->name = "fiolka1c";
    fiolka1c->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1c->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -200.0f, 0.0f };
    fiolka1c->GetComponent<TransformComponent>()->position = glm::vec3{ 22 ,9, -142.64f };

    GameObject * fiolka2 = fiolka2Model->Instantiate(*scena, nullptr, nullptr);
    fiolka2->name = "fiolka2";
    fiolka2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10.000, 10.000, 10.000 };
    fiolka2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka2->GetComponent<TransformComponent>()->position = glm::vec3{ 58.270 ,9.000, -174.380f };
    GameObject * fiolka2b = fiolka2Model->Instantiate(*scena, nullptr, nullptr);
    fiolka2b->name = "fiolka2b";
    fiolka2b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10.000, 10.000, 10.000 };
    fiolka2b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka2b->GetComponent<TransformComponent>()->position = glm::vec3{ 57.630 ,9.000, -171.140f };

    GameObject * ksiazka = ksiazkaModel->Instantiate(*scena, nullptr, nullptr);
    ksiazka->name = "ksiazka";
    ksiazka->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    ksiazka->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    ksiazka->GetComponent<TransformComponent>()->position = glm::vec3{ 56.920 ,8.560, -150.740 };

    GameObject * probowka7 = probowka7Model->Instantiate(*scena, nullptr, nullptr);
    probowka7->name = "probowka7";
    probowka7->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka7->GetComponent<TransformComponent>()->position = glm::vec3{ 57.570 ,9, -174.640f };
    GameObject * probowka7b = probowka7Model->Instantiate(*scena, nullptr, nullptr);
    probowka7b->name = "probowka7b";
    probowka7b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -30.0f, 0.0f };
    probowka7b->GetComponent<TransformComponent>()->position = glm::vec3{ 25.960 ,9, -174.640f };
    GameObject * probowka7c = probowka7Model->Instantiate(*scena, nullptr, nullptr);
    probowka7c->name = "probowka7c";
    probowka7c->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7c->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 45.0f, 0.0f };
    probowka7c->GetComponent<TransformComponent>()->position = glm::vec3{ 21.360 ,9, -182.860 };

    GameObject * probowka6 = probowka6Model->Instantiate(*scena, nullptr, nullptr);
    probowka6->name = "probowka6";
    probowka6->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka6->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -66.900, 0.0f };
    probowka6->GetComponent<TransformComponent>()->position = glm::vec3{ 56.930 ,8.580, -172.450 };
    GameObject * probowka6b = probowka6Model->Instantiate(*scena, nullptr, nullptr);
    probowka6b->name = "probowka6b";
    probowka6b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka6b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -115.900, 0.0f };
    probowka6b->GetComponent<TransformComponent>()->position = glm::vec3{ 56.930 ,8.580, -174.460 };

    GameObject * probowka5 = probowka5Model->Instantiate(*scena, nullptr, nullptr);
    probowka5->name = "probowka5";
    probowka5->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka5->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -45.0f, 0.0f };
    probowka5->GetComponent<TransformComponent>()->position = glm::vec3{ 25.690 ,8.570, -144.030 };
    GameObject * probowka5b = probowka5Model->Instantiate(*scena, nullptr, nullptr);
    probowka5b->name = "probowka5b";
    probowka5b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka5b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -112.0f, 0.0f };
    probowka5b->GetComponent<TransformComponent>()->position = glm::vec3{ 25.880 ,8.570, -147.370 };

    GameObject * probowkaArka_1 = probowkaArka_1_Model->Instantiate(*scena, nullptr, nullptr);
    probowkaArka_1->name = "probowkaArka_1";
    probowkaArka_1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowkaArka_1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -11.300, 0.0f };
    probowkaArka_1->GetComponent<TransformComponent>()->position = glm::vec3{ 57.550 ,9.260, -184.430 };

    GameObject * probowka3 = probowka3Model->Instantiate(*scena, nullptr, nullptr);
    probowka3->name = "probowka3";
    probowka3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.500, 1.500, 1.500 };
    probowka3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -151.800, 0.0f };
    probowka3->GetComponent<TransformComponent>()->position = glm::vec3{ 55.760 ,9.640, -185.700 };

    GameObject * probowka4 = probowka4Model->Instantiate(*scena, nullptr, nullptr);
    probowka4->name = "probowka4";
    probowka4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowka4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka4->GetComponent<TransformComponent>()->position = glm::vec3{ 56.080 ,10.500, -162.990 };
    GameObject * probowka4b = probowka4Model->Instantiate(*scena, nullptr, nullptr);
    probowka4b->name = "probowka4b";
    probowka4b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowka4b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka4b->GetComponent<TransformComponent>()->position = glm::vec3{ 57.500 ,10.500, -161.540 };

    GameObject * labOla1 = labOla1Model->Instantiate(*scena, nullptr, nullptr);
    labOla1->name = "labOla1";
    labOla1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 1, 1 };
    labOla1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    labOla1->GetComponent<TransformComponent>()->position = glm::vec3{ 56.840 ,9.540, -157.110 };

    GameObject * probowka2 = probowka2Model->Instantiate(*scena, nullptr, nullptr);
    probowka2->name = "probowka2";
    probowka2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -176.200, 0.0f };
    probowka2->GetComponent<TransformComponent>()->position = glm::vec3{ 24.200 ,9.400, -145.450 };

    GameObject * folder = folderModel->Instantiate(*scena, nullptr, nullptr);
    folder->name = "folder";
    folder->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    folder->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -93.300, 0.0f };
    folder->GetComponent<TransformComponent>()->position = glm::vec3{ 26.550 ,8.390, -150.040 };

    GameObject * papers = papersModel->Instantiate(*scena, nullptr, nullptr);
    papers->name = "papers";
    papers->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    papers->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    papers->GetComponent<TransformComponent>()->position = glm::vec3{ 22.080 ,8.270, -149.310 };

    GameObject * cup = cupModel->Instantiate(*scena, nullptr, nullptr);
    cup->name = "cup";
    cup->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    cup->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    cup->GetComponent<TransformComponent>()->position = glm::vec3{ 21.060 ,8.420, -186.810 };
    GameObject * cup2 = cupModel->Instantiate(*scena, nullptr, nullptr);
    cup2->name = "cup2";
    cup2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    cup2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -169.600, 0.0f };
    cup2->GetComponent<TransformComponent>()->position = glm::vec3{ 24.210 ,8.420, -185.480 };

    GameObject * corkBoard = corkBoardModel->Instantiate(*scena, nullptr, nullptr);
    corkBoard->name = "corkBoard";
    corkBoard->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    corkBoard->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -90.000, 0.0f };
    corkBoard->GetComponent<TransformComponent>()->position = glm::vec3{ 58.740 ,17.690, -177.210 };

    GameObject * clock = clockModel->Instantiate(*scena, nullptr, nullptr);
    clock->name = "clock";
    clock->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    clock->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 180.000, 0.0f };
    clock->GetComponent<TransformComponent>()->position = glm::vec3{ 58.840 ,17.530, -147.150 };

    GameObject * computer_pbr = computer_pbrModel->Instantiate(*scena, nullptr, nullptr);
    computer_pbr->name = "computer_pbr";
    computer_pbr->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    computer_pbr->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    computer_pbr->GetComponent<TransformComponent>()->position = glm::vec3{ 57.280 ,8.170, -142.870 };
    GameObject * computer_pbr2 = computer_pbrModel->Instantiate(*scena, nullptr, nullptr);
    computer_pbr2->name = "computer_pbr2";
    computer_pbr2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    computer_pbr2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    computer_pbr2->GetComponent<TransformComponent>()->position = glm::vec3{ 57.280 ,8.170, -147.460 };

    GameObject * laboratoryStuff1 = laboratoryStuff1Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff1->name = "laboratoryStuff1";
    laboratoryStuff1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    laboratoryStuff1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -123.800, 0.0f };
    laboratoryStuff1->GetComponent<TransformComponent>()->position = glm::vec3{ 24.490 ,11.490, -157.870 };

    GameObject * laboratoryStuff2 = laboratoryStuff2Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff2->name = "laboratoryStuff2";
    laboratoryStuff2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2.5, 2.5, 2.5 };
    laboratoryStuff2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -72.300, 0.0f };
    laboratoryStuff2->GetComponent<TransformComponent>()->position = glm::vec3{ 23.630 ,10.250, -165.330 };

    GameObject * laboratoryStuff3 = laboratoryStuff3Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff3->name = "laboratoryStuff3";
    laboratoryStuff3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    laboratoryStuff3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 7.500, 0.0f };
    laboratoryStuff3->GetComponent<TransformComponent>()->position = glm::vec3{ 23.540 ,8.540, -178.320 };

    if (cabState.button) {
        ColliderComponent* btnCol = cabState.button->AddComponent<ColliderComponent>();
        btnCol->halfSize = glm::vec3{ 1.0f, 1.0f, 1.0f };

        TransformComponent* btnTr = cabState.button->GetComponent<TransformComponent>();
        cabState.buttonStartPos  = btnTr->position;
        cabState.buttonTargetPos = cabState.buttonStartPos + glm::vec3{ 0.0f, 0.0f, -0.15f };

        cabinetsMap[cabState.button] = cabState;
    }

    glm::vec3 bossRoomCenter = glm::vec3(-35.0f, 5.3f, -190.0f);

    // kapsuła

    //GameObject* capsuleObj = CreateStaticObject(
    //    scena,
    //    bossCapsuleModel.get(),
    //    nullptr,
    //    "BossCapsule",
    //    bossRoomCenter,
    //    glm::vec3(3.0f),
    //    std::nullopt,
    //    glm::vec3(3.0f, 8.0f, 3.0f),
    //    true
    //);

    GameObject* bossObj = bossModel->Instantiate(*scena, nullptr, nullptr);
    bossObj->name = "DemonBoss";

    TransformComponent* bossTr = bossObj->GetComponent<TransformComponent>();
    bossTr->position = glm::vec3(bossRoomCenter.x - 2.2f, bossRoomCenter.y + 8.7f, bossRoomCenter.z - 3.9f);
    bossTr->scale = glm::vec3(3.0f);
    bossTr->rotation = glm::vec3(0.0f, -140.0f, 0.0f);
    bossTr->isDirty = true;

    AnimatorComponent* bossAnimator = bossObj->AddComponent<AnimatorComponent>();

    if (bossModel->rootModel && !bossModel->rootModel->animations.empty()) {
        AnimationClip* defaultBossClip = &bossModel->rootModel->animations[0];

        AnimationHelper::Play(bossAnimator, defaultBossClip, true, 1.0f);
    }
}

void createNuclearRooom(Scene* scena) {
    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaNucearRoom", glm::vec3(-120, 0, -180),  glm::vec3(60, 1, 80));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitATOM",         glm::vec3(-120, 20, -180), glm::vec3(60, 1, 80));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaKoncowaAtom", glm::vec3(-180, 0, -180),  glm::vec3(80, 50, 1), std::nullopt, glm::vec3(1, 50, 80));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaATOMPrawa",   glm::vec3(-120.180, 0, -259.680), glm::vec3(60, 50, 1), std::nullopt, glm::vec3(60, 100, 1));
}

void createCrematorium(Scene* scena) {

    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaKrematorium",   glm::vec3(120, 0, -180),  glm::vec3(60, 1, 80));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitCrematorium",     glm::vec3(120, 25, -180), glm::vec3(60, 1, 80));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaKoncowaKrematorium", glm::vec3(180, 0, -180), glm::vec3(80, 50, 1), std::nullopt, glm::vec3(1, 50, 80));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaKremLewa",       glm::vec3(120.180, 0, -259.680), glm::vec3(60, 50, 1), std::nullopt, glm::vec3(60, 100, 1));

    crematoriumPuzzle.spacingHorizontal = 6.0f;
    crematoriumPuzzle.spacingVertical   = 4.5f;

    crematoriumPuzzle.minExtensionDistance = 9.5f;
    crematoriumPuzzle.maxExtensionDistance = 34.0f;

    crematoriumPuzzle.coffinDimensions = glm::vec3(1.25f, 1.0f, 30.0f);

    crematoriumPuzzle.wallOffset = 17.0f;

    crematoriumPuzzle.w1_buildDirX  = -1.0f;
    crematoriumPuzzle.w1_extendDirZ =  1.0f;
    crematoriumPuzzle.w2_buildDirZ  =  1.0f;
    crematoriumPuzzle.w2_extendDirX = -1.0f;

    glm::vec3 cornerPosition(175.0f, 4.0f, -255.0f);

    if (ruraModel != nullptr && panelModel != nullptr && ruraModel->rootModel != nullptr) {
        crematoriumPuzzle.Init(scena, ruraModel->rootModel, panelModel.get(), nullptr, cornerPosition);
    }
    else {
        //spdlog::error("Model rury albo panelu nie zostal poprawnie zaladowany!");
    }
}

void createRentgenRoom(Scene* scena) {
    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaRentgenRoom",    glm::vec3(0.040, 0, -257.800),  glm::vec3(60, 1, 40));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitRentgen",          glm::vec3(0.040, 20, -257.800), glm::vec3(60, 1, 40));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "KoncowaScianaRentgen",  glm::vec3(0, 0, -297),          glm::vec3(61, 50, 1));


    /*if (i == 2 || i == 3) {
        tablicaKibli[i] = urinModel->Instantiate(*scena1, nullptr, ourShader.get());
        tablicaKibli[i]->name = "Kibel" + std::to_string(i);
        tablicaKibli[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 12, 12, 12 };
        tablicaKibli[i]->AddComponent<RigidbodyComponent>();
        tablicaKibli[i]->AddComponent<ColliderComponent>();
        tablicaKibli[i]->GetComponent<RigidbodyComponent>()->useGravity = false;
        tablicaKibli[i]->GetComponent<RigidbodyComponent>()->isStatic   = true;
        tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 2.5, 4, 2.5 };
        tablicaKibli[i]->GetComponent<ColliderComponent>()->offset       = glm::vec3{ 0, 4, 0 };
        tablicaKibli[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 47.6, 2.0f, -25 + (-10 * i) };
        tablicaKibli[i]->GetComponent<TransformComponent>()->rotation    = glm::vec3{ 0, 270, 0 };
        tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
        tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    }*/
    GameObject * objPuzel1 = puzel1->Instantiate(*scena, nullptr, nullptr);
    objPuzel1->name = "puzel1";
    objPuzel1->GetComponent<TransformComponent>()->position = glm::vec3(-10, 3, -280);
    objPuzel1->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    objPuzel1->AddComponent<RigidbodyComponent>();
    objPuzel1->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel1->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel1->AddComponent<ColliderComponent>();
    objectOriginalRotations[objPuzel1] = objPuzel1->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel1);

    GameObject * objPuzel2 = puzel2->Instantiate(*scena, nullptr, nullptr);
    objPuzel2->name = "puzel2";
    objPuzel2->GetComponent<TransformComponent>()->position = glm::vec3(-5, 3, -280);
    objPuzel2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    objPuzel2->AddComponent<RigidbodyComponent>();
    objPuzel2->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel2->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel2->AddComponent<ColliderComponent>();
    objectOriginalRotations[objPuzel2] = objPuzel2->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel2);

    GameObject * objPuzel3 = puzel3->Instantiate(*scena, nullptr, nullptr);
    objPuzel3->name = "puzel3";
    objPuzel3->GetComponent<TransformComponent>()->position = glm::vec3(0, 3, -280);
    objPuzel3->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    objPuzel3->AddComponent<RigidbodyComponent>();
    objPuzel3->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel3->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel3->AddComponent<ColliderComponent>();
    objectOriginalRotations[objPuzel3] = objPuzel3->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel3);

    GameObject * objPuzel4 = puzel4->Instantiate(*scena, nullptr, nullptr);
    objPuzel4->name = "puzel4";
    objPuzel4->GetComponent<TransformComponent>()->position = glm::vec3(5, 3, -280);
    objPuzel4->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    objPuzel4->AddComponent<RigidbodyComponent>();
    objPuzel4->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel4->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel4->AddComponent<ColliderComponent>();
    objectOriginalRotations[objPuzel4] = objPuzel4->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel4);

    GameObject * objPuzel5 = puzel5->Instantiate(*scena, nullptr, nullptr);
    objPuzel5->name = "puzel5";
    objPuzel5->GetComponent<TransformComponent>()->position = glm::vec3(10, 3, -280);
    objPuzel5->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    objPuzel5->AddComponent<RigidbodyComponent>();
    objPuzel5->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel5->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel5->AddComponent<ColliderComponent>();
    objectOriginalRotations[objPuzel5] = objPuzel5->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel5);

    GameObject * objPuzel6 = puzel6->Instantiate(*scena, nullptr, nullptr);
    objPuzel6->name = "puzel6";
    objPuzel6->GetComponent<TransformComponent>()->position = glm::vec3(15, 3, -280);
    objPuzel6->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    objPuzel6->AddComponent<RigidbodyComponent>();
    objPuzel6->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel6->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel6->AddComponent<ColliderComponent>();
    objectOriginalRotations[objPuzel6] = objPuzel6->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel6);

    GameObject * CzerwonaTablica = czerwonaTablica->Instantiate(*scena, nullptr, nullptr);
    CzerwonaTablica->name = "CzerwonaTablica";
    CzerwonaTablica->GetComponent<TransformComponent>()->position = glm::vec3(-40, 12, -296);
    CzerwonaTablica->GetComponent<TransformComponent>()->rotation = glm::vec3(180, 90, 90);
    CzerwonaTablica->GetComponent<TransformComponent>()->scale = glm::vec3(6, 0.1, 10);
    CzerwonaTablica->AddComponent<RigidbodyComponent>();
    CzerwonaTablica->GetComponent<RigidbodyComponent>()->useGravity = false;
    CzerwonaTablica->GetComponent<RigidbodyComponent>()->isStatic = true;

    GameObject * ZielonaTablica = zielonaTablica->Instantiate(*scena, nullptr, nullptr);
    ZielonaTablica->name = "ZielonaTablica";
    ZielonaTablica->GetComponent<TransformComponent>()->position = glm::vec3(40, 12, -296);
    ZielonaTablica->GetComponent<TransformComponent>()->rotation = glm::vec3(180, 90, 90);
    ZielonaTablica->GetComponent<TransformComponent>()->scale = glm::vec3(6, 0.1, 10);
    ZielonaTablica->AddComponent<RigidbodyComponent>();
    ZielonaTablica->GetComponent<RigidbodyComponent>()->useGravity = false;
    ZielonaTablica->GetComponent<RigidbodyComponent>()->isStatic = true;

    GameObject * rentgen = Rentgen->Instantiate(*scena, nullptr, nullptr);
    rentgen->name = "RentgenTablica";
    rentgen->GetComponent<TransformComponent>()->position = glm::vec3(4, 12, -296);
    rentgen->GetComponent<TransformComponent>()->rotation = glm::vec3(180, 90, 90);
    rentgen->GetComponent<TransformComponent>()->scale = glm::vec3(6, 0.1, 10);
    rentgen->AddComponent<RigidbodyComponent>();
    rentgen->GetComponent<RigidbodyComponent>()->useGravity = false;
    rentgen->GetComponent<RigidbodyComponent>()->isStatic = true;


    int puzzleLightIndex = 4;
    auto createPuzzleSlot = [&](const glm::vec3& pos, const glm::vec3& targetRot, GameObject* expected) {
        GameObject* slotGO = scena->CreateGameObject(nullptr);
        slotGO->name = "PuzzleSlot_" + expected->name;

        TransformComponent* tr = slotGO->AddComponent<TransformComponent>();
        tr->position = pos;

        ColliderComponent* col = slotGO->AddComponent<ColliderComponent>();
        col->halfSize  = glm::vec3(2, 2, 0.5);
        col->isTrigger = true;

        RigidbodyComponent* rb = slotGO->AddComponent<RigidbodyComponent>();
        rb->useGravity = false;
        rb->isStatic   = true;

        GameObject* lightGO = scena->CreateGameObject(nullptr);
        lightGO->name = "PuzzleLight_" + expected->name;

        TransformComponent* lightTr = lightGO->AddComponent<TransformComponent>();
        lightTr->position = pos + glm::vec3(0.0f, 2.0f, 1.0);

        LightComponent* light = lightGO->AddComponent<LightComponent>();
        light->type      = Point;
        light->index = puzzleLightIndex++;
        light->isOn      = false;
        light->ambient   = glm::vec3(0.0f);
        light->diffuse   = glm::vec3(0.0f);
        light->specular  = glm::vec3(0.0f);
        light->constant  = 15.0f;
        light->linear    = 0.3f;
        light->quadratic = 0.05f;
        light->range     = 15.0f;
        light->intensity = 20;

        PuzzleSlot slot;
        slot.targetRotation = targetRot;
        slot.slotObject     = slotGO;
        slot.expectedObject = expected;
        slot.lightObject    = lightGO;
        puzzleSlotsMap[slotGO] = slot;
    };

    createPuzzleSlot(glm::vec3(-2, 15, -295.9), glm::vec3(180, 90,  90), objPuzel2);
    createPuzzleSlot(glm::vec3(4, 15, -295.9), glm::vec3(180, 90, 90), objPuzel6);
    createPuzzleSlot(glm::vec3(11, 15, -295.9), glm::vec3(180, 90, 90), objPuzel4);
    createPuzzleSlot(glm::vec3(-2, 9, -295.9), glm::vec3(180, 90, 90), objPuzel1);
    createPuzzleSlot(glm::vec3(4, 9, -295.9), glm::vec3(180, 90, 90), objPuzel3);
    createPuzzleSlot(glm::vec3(11, 9, -295.9), glm::vec3(180, 90, 90), objPuzel5);
}