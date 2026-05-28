// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#define STB_IMAGE_IMPLEMENTATION  
//#include <stb_image.h>

#include <glad/glad.h>  // Initialize with gladLoadGL()
#include <GLFW/glfw3.h> // Include glfw3.h after our OpenGL definitions
#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <unused/camera.h>
#include <model.h>
#include <prefab.h>
#include <filesystem>

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


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool init();
void init_imgui();


void compileShader();

void input();
void update();
//void render();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

//void createHouse();
void processCameraInput(ECS& ecs, CameraComponent& cam, TransformComponent& transform,
    const std::string& up,
    const std::string& down,
    const std::string& left,
    const std::string& right);


void processCameraGamepad(ECS& ecs,
    CameraComponent& cam,
    TransformComponent& transform,
    int gamepad_id);


void imgui_begin();
void imgui_render(SceneManager& sceneManager);
void imgui_end();

void end_frame();

constexpr int32_t WINDOW_WIDTH = 1920;
constexpr int32_t WINDOW_HEIGHT = 1080;

GLFWwindow* window = nullptr;

// Change these to lower GL version like 4.5 if GL 4.6 can't be initialized on your machine
const     char* glsl_version = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

// camera
//Camera camera(glm::vec3(0.0f, 20.0f, 50.0f));
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseMove = false;
glm::vec3 cameraOffset = glm::vec3(0.0f, 10.0f, -20.0f);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;


bool   show_demo_window = true;
bool   show_another_window = false;
bool wireframeMode = false;
bool focused = false;
int sphereRings = 10;
int sphereSectors = 10;
float sphereRadius = 1.0f;

float cameraDistance = 50.0f;
float  rotationX = 0.0f;
float  rotationY = 0.0f;

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
bool   autoRotation = false;

unsigned int cubemapTexture;
unsigned int skyboxVAO;

GLuint VBO;
GLuint VAO;
GLuint texture;
std::unique_ptr<Prefab> sunModel;
std::unique_ptr<Shader> ourShader;
//std::unique_ptr<Shader> sphereShader;
std::unique_ptr<Shader> skyboxShader;
std::unique_ptr<Shader> reflectShader;
std::unique_ptr<Shader> refractShader;

std::unique_ptr<Prefab> bed1Model;
std::unique_ptr<Prefab> bed2Model;
std::unique_ptr<Prefab> bed3Model;
std::unique_ptr<Prefab> corkBoardModel;
std::unique_ptr<Prefab> cupModel;
std::unique_ptr<Prefab> deskModel;
std::unique_ptr<Prefab> doorsModel;
std::unique_ptr<Prefab> folderModel;
std::unique_ptr<Prefab> krzesloModel;
std::unique_ptr<Prefab> ksiazkaModel;
std::unique_ptr<Prefab> lampa1Model;
std::unique_ptr<Prefab> lampa2Model;
std::unique_ptr<Prefab> lampa3Model;
std::unique_ptr<Prefab> needleModel;
std::unique_ptr<Prefab> bad1Model;
std::unique_ptr<Prefab> bad2Model;
std::unique_ptr<Prefab> bad3Model;
std::unique_ptr<Prefab> papersModel;
std::unique_ptr<Prefab> bossModel;
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
std::unique_ptr<Prefab> toiletPaperModel;
std::unique_ptr<Prefab> mirrorModel1;
std::unique_ptr<Prefab> mirrorModel2;
std::unique_ptr<Prefab> mirrorModel3;
std::unique_ptr<Prefab> washroomExit;
//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
std::unique_ptr<Prefab> floorModel;
std::unique_ptr<Prefab> wallModel;
std::unique_ptr<Prefab> wallModel2;
std::unique_ptr<Prefab> wallModel3;

//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

//EEEEEEEEEEEEEEEEEEEEEEE

std::unique_ptr<Prefab> dyingModelPrefab;
std::unique_ptr<Prefab> jumpSkeletonPrefab;

//OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO


//testowe obiekty do postprocessingu
std::unique_ptr<Prefab> RedModel;
std::unique_ptr<Prefab> BlueModel;
std::unique_ptr<Prefab> GreenModel;

//tu sie one koncza

//std::unique_ptr<Model> sphereVenusModel;
//
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
    float logicTime = 0.0f;
    float inputTime = 0.0f;
};
PerformanceData perf;
RenderSystem * renderSystem = nullptr;
PostProcessingSystem* postProcessingSystem = nullptr;

//meeded for interaction
GameObject* tablicaPapierowKibel[6];
std::unordered_set<GameObject*> rotatableObjects;
std::unordered_set<GameObject*> unlockedDoors;
std::unordered_set<GameObject*> majorDoors;
bool can_open_door_1 = false;

struct DoorState {
    bool isOpen = false;
    float currentAngle = 0.0f;
    float closedAngle = 0.0f;
    float openAngle = 90.0f;
    float targetAngle = 0.0f;
    GameObject* hinge = nullptr;
    glm::vec3 originalOffset = glm::vec3(0.0f);
};
std::unordered_map<GameObject*, DoorState> toiletDoorsMap;
std::unordered_set<GameObject*> pickupObjects;
GameObject* p1HeldObject = nullptr;
GameObject* p2HeldObject = nullptr;

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
    if (focused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if (!focused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

}

GameObject* CreateRaycastTestObject(
    Scene& scene,
    Prefab& prefab,
    Shader* shader,
    const glm::vec3& position,
    const glm::vec3& scale = glm::vec3(1.0f)
) {
    GameObject* go = prefab.Instantiate(scene, nullptr, shader);

    auto* tr = go->GetComponent<TransformComponent>();
    tr->position = position;
    tr->scale    = scale;
    tr->isDirty  = true;

    auto* col = go->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(1.0f);
    col->offset   = glm::vec3(0.0f);

    auto* rc = go->AddComponent<RaycastComponent>();
    rc->range        = 5000.0f;
    rc->fovRayCount  = 200;
    rc->fovAngle     = 200.0f;
    rc->originOffset = glm::vec3(0.0f, 0.5f, 0.0f);
    rc->debugDraw    = false;

    return go;
}


void processCameraInput(ECS& ecs, CameraComponent& cam, TransformComponent& transform,
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
        //glm::vec3 pos = TransformHelper::getGlobalPosition(transform);
        //TransformHelper::setGlobalPosition(transform, pos, camera.GetParent()->GetComponent<TransformComponent>());

        transform.position += dir * MovementSpeed * deltaTime;
        transform.isDirty = true;
    }
}

void processCameraMouse(ECS& ecs, CameraComponent& cam, TransformComponent& transform)
{
    const auto& hid = ecs.GetSystem<HID>();
    float dx = hid->get_mouse_dx();
    float dy = hid->get_mouse_dy();

    CameraHelper::ProcessMouseMovement(cam, transform, dx, dy);
}
void addAllSystems(ECS& ecs);
void connectAllModels();

void HandlePlayerInteraction(
    ECS& ecs,
    const std::string& inputAction,
    RaycastComponent* playerRaycast,
    GameObject* playerCamera,
    GameObject*& myHeldObject,
    GameObject* otherPlayerHeldObject,
    Scene* scene,
    std::unordered_map<GameObject*, float>& rotatingObjects
) {
    if (!ecs.GetSystem<HID>()->is_action_just_pressed(inputAction)) return;

    if (myHeldObject != nullptr) {
        // Upuszczanie
        myHeldObject->SetParent(scene->GetRoot());
        TransformComponent* camTr = playerCamera->GetComponent<TransformComponent>();
        TransformComponent* heldTr = myHeldObject->GetComponent<TransformComponent>();
        CameraComponent* camComp = playerCamera->GetComponent<CameraComponent>();

        heldTr->position = camTr->position + (camComp->state.Front * 3.0f);
        heldTr->rotation = glm::vec3(0.0f);
        heldTr->isDirty = true;

        if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
            rb->useGravity = true;
            rb->isStatic = false;
            rb->velocity = glm::vec3(0.0f);
        }
        myHeldObject = nullptr;
    }
    else if (playerRaycast->anyHit()) {
        RaycastHit hit = playerRaycast->closestHit();
        if (hit.hitObject != nullptr) {
            // Obracanie
            if (rotatableObjects.count(hit.hitObject)) {
                if (!rotatingObjects.count(hit.hitObject)) rotatingObjects[hit.hitObject] = 60.0f;
            }
            //PAIN
            if (majorDoors.count(hit.hitObject)) {
                if (can_open_door_1) {
                    for (GameObject* door : majorDoors) {
                        TransformComponent* transform = door->GetComponent<TransformComponent>();
                        if (transform != nullptr) {
                            transform->position = glm::vec3(-1000.0f, -1000.0f, -1000.0f);
                        }
                    }
                }
            }


            // Otwieranie drzwi
            else if (toiletDoorsMap.count(hit.hitObject)) {
                DoorState& state = toiletDoorsMap[hit.hitObject];
                state.isOpen = !state.isOpen;
                state.targetAngle = state.isOpen ? state.openAngle : state.closedAngle;

                if (auto col = hit.hitObject->GetComponent<ColliderComponent>()) {
                    col->halfSize = state.isOpen ? glm::vec3{ 1.0f, 10.0f, 1.0f } : glm::vec3{ 0.8f, 10.0f, 4.0f };
                    col->offset = state.isOpen ? glm::vec3(0.0f) : state.originalOffset;
                }
            }
            // Podnoszenie (zabezpieczone przed wyrwaniem obiektu drugiemu graczowi)
            else if (pickupObjects.count(hit.hitObject) && hit.hitObject != otherPlayerHeldObject) {
                myHeldObject = hit.hitObject;
                myHeldObject->SetParent(playerCamera);

                TransformComponent* heldTr = myHeldObject->GetComponent<TransformComponent>();
                heldTr->position = glm::vec3(1.0f, -1.0f, -3.0f);
                heldTr->rotation = glm::vec3(0.0f);
                heldTr->isDirty = true;

                if (auto rb = myHeldObject->GetComponent<RigidbodyComponent>()) {
                    rb->useGravity = false;
                    rb->isStatic = true;
                }
            }
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

GameObject* CreateInteractableDoor(Scene* scene, Prefab* prefab, Shader* shader, const std::string& name, const glm::vec3& position, const glm::vec3& scale, const glm::vec3& pivotOffset, const glm::vec3& colliderHalfSize, float openAngle) {
    GameObject* hinge = scene->CreateGameObject(nullptr);
    hinge->name = "Hinge_" + name;
    TransformComponent* hingeTr = hinge->AddComponent<TransformComponent>();
    hingeTr->position = position + pivotOffset;

    GameObject* door = prefab->Instantiate(*scene, hinge, shader);
    door->name = name;
    TransformComponent* doorTr = door->GetComponent<TransformComponent>();
    doorTr->scale = scale;
    doorTr->rotation = glm::vec3(0.0f, 90.0f, 0.0f);
    doorTr->position = -pivotOffset;

    hinge->AddComponent<RigidbodyComponent>()->useGravity = false;
    hinge->GetComponent<RigidbodyComponent>()->isStatic = true;
    ColliderComponent* col = hinge->AddComponent<ColliderComponent>();
    col->halfSize = colliderHalfSize;
    col->offset = -pivotOffset;
    col->isWalkable = false;
    col->affectsNavMesh = true;

    DoorState state;
    state.hinge = hinge;
    state.openAngle = openAngle;
    state.originalOffset = -pivotOffset;
    toiletDoorsMap[hinge] = state;
    return hinge;
}

void createFirstRoom(Scene* scena1) {
    //pokoj bedzie tu
    floorModel = std::make_unique<Prefab>("res/models/number_floor.glb");
    GameObject* groundObject = floorModel->Instantiate(*scena1, nullptr, ourShader.get());
    groundObject->name = "Ground1";
    groundObject->GetComponent<TransformComponent>()->scale.x = 100;
    groundObject->GetComponent<TransformComponent>()->scale.y = 1;
    groundObject->GetComponent<TransformComponent>()->scale.z = 100;

    groundObject->AddComponent<RigidbodyComponent>();
    groundObject->AddComponent<ColliderComponent>();

    groundObject->GetComponent<RigidbodyComponent>()->useGravity = false;
    groundObject->GetComponent<RigidbodyComponent>()->isStatic = true;

    groundObject->GetComponent<TransformComponent>()->position = glm::vec3 {10, 0, -50};
    groundObject->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 40, 1, 50 };
    groundObject->GetComponent<ColliderComponent>()->isWalkable = true;
    groundObject->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    GameObject* groundObject2 = floorModel->Instantiate(*scena1, nullptr, ourShader.get());
    groundObject2->name = "Ground2";
    groundObject2->GetComponent<TransformComponent>()->scale.x = 100;
    groundObject2->GetComponent<TransformComponent>()->scale.y = 1;
    groundObject2->GetComponent<TransformComponent>()->scale.z = 100;

    groundObject2->AddComponent<RigidbodyComponent>();
    groundObject2->AddComponent<ColliderComponent>();

    groundObject2->GetComponent<RigidbodyComponent>()->useGravity = false;
    groundObject2->GetComponent<RigidbodyComponent>()->isStatic = true;

    groundObject2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 100, 1, 100 };
    groundObject2->GetComponent<TransformComponent>()->position.x = 0;
    groundObject2->GetComponent<TransformComponent>()->position.y = 0;
    groundObject2->GetComponent<TransformComponent>()->position.z = -200;


    GameObject* groundObject3 = floorModel->Instantiate(*scena1, nullptr, ourShader.get());
    groundObject3->name = "Ground3";
    groundObject3->GetComponent<TransformComponent>()->scale.x = 100;
    groundObject3->GetComponent<TransformComponent>()->scale.y = 1;
    groundObject3->GetComponent<TransformComponent>()->scale.z = 100;

    groundObject3->AddComponent<RigidbodyComponent>();
    groundObject3->AddComponent<ColliderComponent>();

    groundObject3->GetComponent<RigidbodyComponent>()->useGravity = false;
    groundObject3->GetComponent<RigidbodyComponent>()->isStatic = true;

    groundObject3->GetComponent<TransformComponent>()->position.y = 20;

    groundObject3->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 100, 1, 100 };

    GameObject* groundObject4 = floorModel->Instantiate(*scena1, nullptr, ourShader.get());
    groundObject4->name = "Ground4";
    groundObject4->GetComponent<TransformComponent>()->scale.x = 100;
    groundObject4->GetComponent<TransformComponent>()->scale.y = 1;
    groundObject4->GetComponent<TransformComponent>()->scale.z = 100;

    groundObject4->AddComponent<RigidbodyComponent>();
    groundObject4->AddComponent<ColliderComponent>();

    groundObject4->GetComponent<RigidbodyComponent>()->useGravity = false;
    groundObject4->GetComponent<RigidbodyComponent>()->isStatic = true;

    groundObject4->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 100, 1, 100 };
    groundObject4->GetComponent<TransformComponent>()->position.x = 0;
    groundObject4->GetComponent<TransformComponent>()->position.y = 40;
    groundObject4->GetComponent<TransformComponent>()->position.z = -200;


    wallModel = std::make_unique<Prefab>("res/models/wall.glb");
    wallModel2 = std::make_unique<Prefab>("res/models/wall2.glb");
    wallModel3 = std::make_unique<Prefab>("res/models/wall3.glb");
    GameObject* wallObject = wallModel->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject->GetComponent<TransformComponent>()->scale.x = 50;
    wallObject->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject->GetComponent<TransformComponent>()->scale.z = 1;

    wallObject->AddComponent<RigidbodyComponent>();
    wallObject->AddComponent<ColliderComponent>();

    wallObject->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 50, 50, 1 };

    wallObject->GetComponent<TransformComponent>()->position.x = 0;
    wallObject->GetComponent<TransformComponent>()->position.y = 0;
    wallObject->GetComponent<TransformComponent>()->position.z = -10;


    GameObject* wallObject2 = wallModel2->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject2->GetComponent<TransformComponent>()->scale.x = 100;
    wallObject2->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject2->GetComponent<TransformComponent>()->scale.z = 1;


    wallObject2->AddComponent<RigidbodyComponent>();
    wallObject2->AddComponent<ColliderComponent>();

    wallObject2->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject2->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 50, 100 };

    wallObject2->GetComponent<TransformComponent>()->position.x = 50;
    wallObject2->GetComponent<TransformComponent>()->position.y = 0;
    wallObject2->GetComponent<TransformComponent>()->position.z = 0;

    GameObject* wallObject3 = wallModel2->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject3->GetComponent<TransformComponent>()->scale.x = 100;
    wallObject3->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject3->GetComponent<TransformComponent>()->scale.z = 1;


    wallObject3->AddComponent<RigidbodyComponent>();
    wallObject3->AddComponent<ColliderComponent>();

    wallObject3->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject3->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject3->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 50, 100 };

    wallObject3->GetComponent<TransformComponent>()->position.x = -25;
    wallObject3->GetComponent<TransformComponent>()->position.y = 0;
    wallObject3->GetComponent<TransformComponent>()->position.z = 0;


    GameObject* wallObject4 = wallModel->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject4->GetComponent<TransformComponent>()->scale.x = 100;
    wallObject4->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject4->GetComponent<TransformComponent>()->scale.z = 1;

    wallObject4->AddComponent<RigidbodyComponent>();
    wallObject4->AddComponent<ColliderComponent>();

    wallObject4->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject4->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject4->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 100, 100, 1 };

    wallObject4->GetComponent<TransformComponent>()->position.x = 0;
    wallObject4->GetComponent<TransformComponent>()->position.y = 0;
    wallObject4->GetComponent<TransformComponent>()->position.z = -300;


    GameObject* wallObject5 = wallModel2->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject5->GetComponent<TransformComponent>()->scale.x = 100;
    wallObject5->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject5->GetComponent<TransformComponent>()->scale.z = 1;


    wallObject5->AddComponent<RigidbodyComponent>();
    wallObject5->AddComponent<ColliderComponent>();

    wallObject5->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject5->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject5->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 50, 100 };

    wallObject5->GetComponent<TransformComponent>()->position.x = 100;
    wallObject5->GetComponent<TransformComponent>()->position.y = 0;
    wallObject5->GetComponent<TransformComponent>()->position.z = -200;

    GameObject* wallObject6 = wallModel2->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject6->GetComponent<TransformComponent>()->scale.x = 100;
    wallObject6->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject6->GetComponent<TransformComponent>()->scale.z = 1;


    wallObject6->AddComponent<RigidbodyComponent>();
    wallObject6->AddComponent<ColliderComponent>();

    wallObject6->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject6->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject6->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 50, 100 };
    wallObject6->GetComponent<ColliderComponent>()->affectsNavMesh = true;;

    wallObject6->GetComponent<TransformComponent>()->position.x = -100;
    wallObject6->GetComponent<TransformComponent>()->position.y = 0;
    wallObject6->GetComponent<TransformComponent>()->position.z = -200;


    GameObject* wallObject7 = wallModel->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject7->GetComponent<TransformComponent>()->scale.x = 100;
    wallObject7->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject7->GetComponent<TransformComponent>()->scale.z = 1;

    wallObject7->AddComponent<RigidbodyComponent>();
    wallObject7->AddComponent<ColliderComponent>();

    wallObject7->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject7->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject7->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 100, 100, 1 };
    wallObject7->GetComponent<ColliderComponent>()->affectsNavMesh = true;


    wallObject7->GetComponent<TransformComponent>()->position.x = 110;
    wallObject7->GetComponent<TransformComponent>()->position.y = 0;
    wallObject7->GetComponent<TransformComponent>()->position.z = -100;

    GameObject* wallObject8 = wallModel->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject8->GetComponent<TransformComponent>()->scale.x = 100;
    wallObject8->GetComponent<TransformComponent>()->scale.y = 50;
    wallObject8->GetComponent<TransformComponent>()->scale.z = 1;

    wallObject8->AddComponent<RigidbodyComponent>();
    wallObject8->AddComponent<ColliderComponent>();

    wallObject8->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject8->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject8->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 100, 100, 1 };
    wallObject8->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    wallObject8->GetComponent<TransformComponent>()->position.x = -110;
    wallObject8->GetComponent<TransformComponent>()->position.y = 0;
    wallObject8->GetComponent<TransformComponent>()->position.z = -100;

    GameObject* wallObject9 = wallModel->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject9->GetComponent<TransformComponent>()->scale = glm::vec3{ 100, 50, 1 };

    wallObject9->AddComponent<RigidbodyComponent>();
    wallObject9->AddComponent<ColliderComponent>();

    wallObject9->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject9->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject9->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 10, 44, 1 };
    wallObject9->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    wallObject9->GetComponent<TransformComponent>()->position = glm::vec3{ 0, 70 , -100 };

    GameObject* tablicaKibli[6];
    for (int i = 0; i < 6; i++) {
        tablicaKibli[i] = toiletModel->Instantiate(*scena1, nullptr, ourShader.get());
        tablicaKibli[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 1.5, 1.5, 1.5 };
        tablicaKibli[i]->AddComponent<RigidbodyComponent>();
        tablicaKibli[i]->AddComponent<ColliderComponent>();
        tablicaKibli[i]->GetComponent<RigidbodyComponent>()->useGravity = false;
        tablicaKibli[i]->GetComponent<RigidbodyComponent>()->isStatic = true;
        tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 2.5, 4, 2.5 };
        tablicaKibli[i]->GetComponent<ColliderComponent>()->offset = glm::vec3{ 0, 4, 0 };
        tablicaKibli[i]->GetComponent<TransformComponent>()->position = glm::vec3{ 45, 0.5f, -45 + (-10 * i) };
        tablicaKibli[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable = false;
        tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    }
    GameObject* tablicaZaslon[7];
    for (int i = 0; i < 7; i++) {
        tablicaZaslon[i] = wallModel3->Instantiate(*scena1, nullptr, ourShader.get());
        tablicaZaslon[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 0.3, 30, 20 };;
        tablicaZaslon[i]->AddComponent<RigidbodyComponent>();
        tablicaZaslon[i]->AddComponent<ColliderComponent>();
        tablicaZaslon[i]->GetComponent<RigidbodyComponent>()->useGravity = false;
        tablicaZaslon[i]->GetComponent<RigidbodyComponent>()->isStatic = true;
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 20, 15, 0.3 };
        tablicaZaslon[i]->GetComponent<TransformComponent>()->position = glm::vec3{ 50, 0, -40 + (-10 * i) };
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->isWalkable = false;
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    }
    GameObject* tablicaDrzwiczekDoKilba[6];
    for (int i = 0; i < 6; i++) {
        glm::vec3 doorPos = glm::vec3{ 30.0f, 1.0f, -44.0f + (-10.0f * i) };
        glm::vec3 doorScale = glm::vec3{ 11.0f, 10.0f, 16.0f };

        //if (i == 5) {
        //    doorPos = glm::vec3{ 30.0f, 1.0f, -33.5f + (-12.0f * 5) };
        //    doorScale = glm::vec3{ 10.0f, 10.0f, 16.0f };
        //}

        glm::vec3 pivotOffset = glm::vec3(0.2f, 0.0f, 3.8f);
        glm::vec3 colliderSize = glm::vec3{ 0.8f, 10.0f, 4.0f };

        GameObject* hinge = CreateInteractableDoor(
            scena1, doorsToiletModel.get(), ourShader.get(),
            "ToiletDoor_" + std::to_string(i), doorPos, doorScale, pivotOffset, colliderSize, 90.0f
        );
        unlockedDoors.insert(hinge);
    }


    for (int i = 0; i < 6; i++) {
        tablicaPapierowKibel[i] = toiletPaperModel->Instantiate(*scena1, nullptr, ourShader.get());
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 1, 1, 1 };
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaPapierowKibel[i]->AddComponent<RigidbodyComponent>();
        tablicaPapierowKibel[i]->AddComponent<ColliderComponent>();
        tablicaPapierowKibel[i]->GetComponent<RigidbodyComponent>()->useGravity = false;
        tablicaPapierowKibel[i]->GetComponent<RigidbodyComponent>()->isStatic = true;
        tablicaPapierowKibel[i]->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 0.7, 0.7, 0.7 };
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->position = glm::vec3{ 35, 5.0, -40.7 + (-10 * i) };
        rotatableObjects.insert(tablicaPapierowKibel[i]);
    }

    GameObject* tablicaSink[6];
    for (int i = 0; i < 6; i++) {
        tablicaSink[i] = sinkModel->Instantiate(*scena1, nullptr, ourShader.get());
        tablicaSink[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 3, 3, 3 };
        tablicaSink[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaSink[i]->AddComponent<RigidbodyComponent>();
        tablicaSink[i]->AddComponent<ColliderComponent>();
        tablicaSink[i]->GetComponent<RigidbodyComponent>()->useGravity = false;
        tablicaSink[i]->GetComponent<RigidbodyComponent>()->isStatic = true;
        tablicaSink[i]->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 3, 20, 3 };
        tablicaSink[i]->GetComponent<TransformComponent>()->position = glm::vec3{ -21.5, -2.0, -45 + (-10 * i) };
        tablicaSink[i]->GetComponent<ColliderComponent>()->isWalkable = false;
        tablicaSink[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    }
    GameObject* lustro1;
    lustro1 = mirrorModel1->Instantiate(*scena1, nullptr, ourShader.get());
    lustro1->GetComponent<TransformComponent>()->scale = glm::vec3{ 1, 2, 8 };
    lustro1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro1->AddComponent<RigidbodyComponent>();
    lustro1->AddComponent<ColliderComponent>();
    lustro1->GetComponent<RigidbodyComponent>()->useGravity = false;
    lustro1->GetComponent<RigidbodyComponent>()->isStatic = true;
    lustro1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 1, 1 };
    lustro1->GetComponent<TransformComponent>()->position = glm::vec3{ -23.5, 12.0, -50 + (-20 * 0) };
    GameObject* lustro2;
    lustro2 = mirrorModel2->Instantiate(*scena1, nullptr, ourShader.get());
    lustro2->GetComponent<TransformComponent>()->scale = glm::vec3{ 1, 2, 8 };
    lustro2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro2->AddComponent<RigidbodyComponent>();
    lustro2->AddComponent<ColliderComponent>();
    lustro2->GetComponent<RigidbodyComponent>()->useGravity = false;
    lustro2->GetComponent<RigidbodyComponent>()->isStatic = true;
    lustro2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 1, 1 };
    lustro2->GetComponent<TransformComponent>()->position = glm::vec3{ -23.5, 12.0, -50 + (-20 * 1) };
    GameObject* lustro3;
    lustro3 = mirrorModel3->Instantiate(*scena1, nullptr, ourShader.get());
    lustro3->GetComponent<TransformComponent>()->scale = glm::vec3{ 1, 2, 8 };
    lustro3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro3->AddComponent<RigidbodyComponent>();
    lustro3->AddComponent<ColliderComponent>();
    lustro3->GetComponent<RigidbodyComponent>()->useGravity = false;
    lustro3->GetComponent<RigidbodyComponent>()->isStatic = true;
    lustro3->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 1, 1 };
    lustro3->GetComponent<TransformComponent>()->position = glm::vec3{ -23.5, 12.0, -50 + (-20 * 2) };
    GameObject* tablicaDrzwi[2];
    for (int i = 0; i < 2; i++) {
        tablicaDrzwi[i] = washroomExit->Instantiate(*scena1, nullptr, ourShader.get());
        tablicaDrzwi[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 10, 11, 10 };
        tablicaDrzwi[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 180 * i, 0 };
        tablicaDrzwi[i]->AddComponent<RigidbodyComponent>();
        tablicaDrzwi[i]->AddComponent<ColliderComponent>();
        tablicaDrzwi[i]->GetComponent<RigidbodyComponent>()->useGravity = false;
        tablicaDrzwi[i]->GetComponent<RigidbodyComponent>()->isStatic = true;
        tablicaDrzwi[i]->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 5, 22, 1 };
        tablicaDrzwi[i]->GetComponent<TransformComponent>()->position = glm::vec3{ -5 + (10 * i), 0.0, -100 };
        majorDoors.insert(tablicaDrzwi[i]);
        tablicaDrzwi[i]->GetComponent<ColliderComponent>()->isWalkable = false;
        tablicaDrzwi[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
    }

    GameObject* cup = cupModel->Instantiate(*scena1, nullptr, ourShader.get());
    cup->name = "Magiczny_Kubek";

    TransformComponent* cupTr = cup->GetComponent<TransformComponent>();
    cupTr->position = glm::vec3{ 30.0f, 5.0f, -30.0f };
    cupTr->scale = glm::vec3{ 10.0f, 10.0f, 10.0f };

    RigidbodyComponent* cupRb = cup->AddComponent<RigidbodyComponent>();
    cupRb->useGravity = true;
    cupRb->isStatic = false;

    ColliderComponent* cupCol = cup->AddComponent<ColliderComponent>();
    cupCol->halfSize = glm::vec3{ 1.0f, 1.0f, 1.0f };

    pickupObjects.insert(cup);
    //Zostawiam jeśli przyda się w przyszłości
    /*GameObject* wallObject10 = wallModel3->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject10->GetComponent<TransformComponent>()->scale.x = 30;
    wallObject10->GetComponent<TransformComponent>()->scale.y = 10;
    wallObject10->GetComponent<TransformComponent>()->scale.z = 10;

    wallObject10->AddComponent<RigidbodyComponent>();
    wallObject10->AddComponent<ColliderComponent>();

    wallObject10->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject10->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject10->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 10, 10, 30 };
    wallObject10->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    wallObject10->GetComponent<TransformComponent>()->position.x = 30;
    wallObject10->GetComponent<TransformComponent>()->position.y = 0;
    wallObject10->GetComponent<TransformComponent>()->position.z = 0;

    GameObject* wallObject11 = wallModel3->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject11->GetComponent<TransformComponent>()->scale.x = 30;
    wallObject11->GetComponent<TransformComponent>()->scale.y = 10;
    wallObject11->GetComponent<TransformComponent>()->scale.z = 10;

    wallObject11->AddComponent<RigidbodyComponent>();
    wallObject11->AddComponent<ColliderComponent>();

    wallObject11->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject11->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject11->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 10, 10, 30 };
    wallObject11->GetComponent<ColliderComponent>()->affectsNavMesh = true;


    wallObject11->GetComponent<TransformComponent>()->position.x = -30;
    wallObject11->GetComponent<TransformComponent>()->position.y = 0;
    wallObject11->GetComponent<TransformComponent>()->position.z = 0;

    GameObject* wallObject12 = wallModel3->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject12->GetComponent<TransformComponent>()->scale.x = 30;
    wallObject12->GetComponent<TransformComponent>()->scale.y = 10;
    wallObject12->GetComponent<TransformComponent>()->scale.z = 10;

    wallObject12->AddComponent<RigidbodyComponent>();
    wallObject12->AddComponent<ColliderComponent>();

    wallObject12->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject12->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject12->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 10, 10, 30 };
    wallObject12->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    wallObject12->GetComponent<TransformComponent>()->position.x = 30;
    wallObject12->GetComponent<TransformComponent>()->position.y = 0;
    wallObject12->GetComponent<TransformComponent>()->position.z = -200;

    GameObject* wallObject13 = wallModel3->Instantiate(*scena1, nullptr, ourShader.get());
    wallObject13->GetComponent<TransformComponent>()->scale.x = 30;
    wallObject13->GetComponent<TransformComponent>()->scale.y = 10;
    wallObject13->GetComponent<TransformComponent>()->scale.z = 10;

    wallObject13->AddComponent<RigidbodyComponent>();
    wallObject13->AddComponent<ColliderComponent>();

    wallObject13->GetComponent<RigidbodyComponent>()->useGravity = false;
    wallObject13->GetComponent<RigidbodyComponent>()->isStatic = true;

    wallObject13->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 10, 10, 30 };
    wallObject13->GetComponent<ColliderComponent>()->affectsNavMesh = true;

    wallObject13->GetComponent<TransformComponent>()->position.x = -30;
    wallObject13->GetComponent<TransformComponent>()->position.y = 0;
    wallObject13->GetComponent<TransformComponent>()->position.z = -200;*/
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

    auto* tr     = go->GetComponent<TransformComponent>();
    tr->position = homePos;
    tr->scale    = glm::vec3(0.3f);
    tr->isDirty  = true;

    auto* rb = go->AddComponent<RigidbodyComponent>();
    rb->useGravity = true;
    rb->isStatic   = false;

    auto* col = go->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(0.3f, 0.2f, 0.3f);

    auto* nav      = go->AddComponent<NavPathComponent>();
    nav->state     = NavAgentState::ExternalControl;
    nav->moveSpeed = moveSpeed;
    nav->idleTimeMax = 0.0f;

    auto* leader = go->AddComponent<CockroachLeaderComponent>();
    leader->homePosition     = homePos;
    leader->homeRadius       = 15.0f;
    leader->homeTimeRequired = 8.0f;
    leader->exploreRadius    = 50.0f;
    leader->exploreDuration  = 20.0f;
    leader->detectionRadius  = 25.0f;
    leader->escapeRadius     = 35.0f;
    leader->idleWanderRadius = 8.0f;
    leader->state            = LeaderState::Idle;

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

    auto* tr     = go->GetComponent<TransformComponent>();
    tr->position = spawnPos;
    tr->scale    = glm::vec3(0.25f);
    tr->isDirty  = true;

    auto* rb = go->AddComponent<RigidbodyComponent>();
    rb->useGravity = true;
    rb->isStatic   = false;

    auto* col = go->AddComponent<ColliderComponent>();
    col->halfSize = glm::vec3(0.25f, 0.15f, 0.25f);

    auto* nav      = go->AddComponent<NavPathComponent>();
    nav->state     = NavAgentState::ExternalControl;
    nav->moveSpeed = moveSpeed;
    nav->idleTimeMax = 0.0f;

    auto* follower = go->AddComponent<CockroachFollowerComponent>();
    follower->leaderGameObject  = leaderGO;
    follower->followDistance    = 6.0f;
    follower->followStopDistance= 2.0f;
    follower->idleWanderRadius  = 6.0f;
    follower->state             = FollowerState::Follow;

    return go;
}

int main(int, char**)
{
    if (!init())
    {
        spdlog::error("Failed to initialize project!");
        return EXIT_FAILURE;
    }
    spdlog::info("Initialized project.");

    init_imgui();
    spdlog::info("Initialized ImGui.");

    ECS ecs;
    SceneManager sceneManager;

    sceneManager.CreateScene("Scena 1", ecs);

    Scene* scena1 = sceneManager.GetActiveScene();

    addAllSystems(ecs);

    ourShader = std::make_unique<Shader>("res/shaders/basic.vert", "res/shaders/basic.frag");
    ourShader->use();

    groundModel = std::make_unique<Prefab>("res/models/podloze.glb");
    sunModel = std::make_unique<Prefab>("res/models/Sun.glb");

    GameObject* obb3 = sunModel->Instantiate(*scena1, nullptr, ourShader.get());
    obb3->GetComponent<TransformComponent>()->scale = glm::vec3(25.0f);
    obb3->GetComponent<TransformComponent>()->position = glm::vec3(75.0f, 250.0f, 0.0f);

    obb3->AddComponent<RigidbodyComponent>()->useGravity = false;
    obb3->AddComponent<ColliderComponent>()->halfSize = glm::vec3{ 25, 25, 25 };

    GLuint diff = ResourceManager::LoadTexture("diffuse_brick.png", "res/textures/");
    GLuint spec = ResourceManager::LoadTexture("specular_brick.png", "res/textures/");
    GLuint norm = ResourceManager::LoadTexture("normal_brick.png", "res/textures/");

    auto brickMat = std::make_shared<Material>();
    brickMat->shader = ourShader.get();
    brickMat->diffuseMap = diff;
    brickMat->specularMap = spec;
    brickMat->normalMap = norm;
    brickMat->shininess = 64.0f;

    RenderHelper::SetMaterial(obb3, brickMat);

    GameObject* camera1 = scena1->CreateGameObject(nullptr);
    CameraComponent* camCompLeft = camera1->AddComponent<CameraComponent>();
    ColliderComponent* camera1collider = camera1->AddComponent<ColliderComponent>();
    RigidbodyComponent* rigidBodyCamera1 = camera1->AddComponent<RigidbodyComponent>();
    RaycastComponent* player1Raycast = camera1->AddComponent<RaycastComponent>();
    player1Raycast->debugDraw = false;

    camera1->AddComponent<LightComponent>();
    LightComponent* light2 = camera1->GetComponent<LightComponent>();


    light2->type = Spot;
    light2->index = 0;

    light2->ambient = glm::vec3(0.25f);
    light2->diffuse = glm::vec3(1.0f);
    light2->specular = glm::vec3(1.0f);

    light2->constant = 1.0f;
    light2->linear = 0.10f;
    light2->quadratic = 0.00001f;

    light2->cutOff = glm::cos(glm::radians(4.0f));
    light2->outerCutOff = glm::cos(glm::radians(16.0f));

    camera1->GetComponent<RigidbodyComponent>()->useGravity = false;
    camera1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.0f, 10.0f, 1.0f };

    GameObject* camera2 = scena1->CreateGameObject(nullptr);
    CameraComponent* camCompRight = camera2->AddComponent<CameraComponent>();
    ColliderComponent* camera2collider = camera2->AddComponent<ColliderComponent>();
    RigidbodyComponent* rigidBodyCamera2 = camera2->AddComponent<RigidbodyComponent>();
    camera2->GetComponent<RigidbodyComponent>()->useGravity = false;
    camera2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.0f, 10.0f, 1.0f };
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

    light3->cutOff = glm::cos(glm::radians(4.0f));
    light3->outerCutOff = glm::cos(glm::radians(16.0f));

    TransformComponent* camTransform1 = camera1->GetComponent<TransformComponent>();
    camTransform1->position = glm::vec3(0.0f, 20.0f, -20.0f);
    CameraHelper::InitialCamera(*camCompLeft, *camTransform1,
        glm::vec3(0.0f, 1.0f, 0.0f),
        YAW,
        PITCH,
        Viewport{ 0.0f, 0.0f, 0.5f, 1.0f }
    );


    camCompLeft->isActive = true;

    TransformComponent* camTransform2 = camera2->GetComponent<TransformComponent>();

    camTransform2->position = glm::vec3(0.0f, 20.0f, -20.0f);
    CameraHelper::InitialCamera(*camCompRight, *camTransform2,
        glm::vec3(0.0f, 1.0f, 0.0f),
        0.0f, -20.0f,
        Viewport{ 0.5f, 0.0f, 0.5f, 1.0f }
    );

    camCompRight->isActive = true;

    /*
    GameObject* obj_Sprite_2 = scena1->CreateGameObject(nullptr);
    SpriteComponent* sprite_2 = obj_Sprite_2->AddComponent<SpriteComponent>();

    sprite_2->isAnimating = true;
    sprite_2->loop = true;
    sprite_2->sprites.push_back(
        ResourceManager::LoadTexture("Face_1.png", "res/textures/PGK_placeholders")
    );
    sprite_2->sprites.push_back(
        ResourceManager::LoadTexture("Face_2.png", "res/textures/PGK_placeholders")
    );
    sprite_2->sprites.push_back(
        ResourceManager::LoadTexture("Face_3.png", "res/textures/PGK_placeholders")
    );
    sprite_2->screenPosition = glm::vec2(0.0f, 128.0f);
    sprite_2->size = glm::vec2(128.0f, 128.0f);
    sprite_2->frameDuration = 0.5f;
    */

    GameObject* player1InteractionInfo_obj = scena1->CreateGameObject(nullptr);
    SpriteComponent* player1InteractionInfo = player1InteractionInfo_obj->AddComponent<SpriteComponent>();
    player1InteractionInfo->textEnabled = true;
    player1InteractionInfo->screenPosition = glm::vec2(480.0f, 640.0f);
    player1InteractionInfo->text = "";
    player1InteractionInfo->textOutlineEnabled = true;
    player1InteractionInfo->textCentered = true;
    player1InteractionInfo->layer = 1;

    GameObject* player2InteractionInfo_obj = scena1->CreateGameObject(nullptr);
    SpriteComponent* player2InteractionInfo = player2InteractionInfo_obj->AddComponent<SpriteComponent>();
    player2InteractionInfo->textEnabled = true;
    player2InteractionInfo->screenPosition = glm::vec2(1440.0f, 640.0f);
    player2InteractionInfo->text = "";
    player2InteractionInfo->textOutlineEnabled = true;
    player2InteractionInfo->textCentered = true;
    player2InteractionInfo->layer = 1;

    connectAllModels();

    GameObject* model1 = bed1Model->Instantiate(*scena1, nullptr, ourShader.get());
    model1->GetComponent<TransformComponent>()->position.x = 0.0f;
    model1->GetComponent<TransformComponent>()->position.y = 100.0f;
    model1->GetComponent<TransformComponent>()->position.z = 20.0f;

    model1->AddComponent<RigidbodyComponent>();
    model1->AddComponent<ColliderComponent>();
    model1->AddComponent<LightComponent>();

    model1->GetComponent<LightComponent>()->type = Directional;
    model1->GetComponent<LightComponent>()->index = 0;
    auto* light = model1->GetComponent<LightComponent>();

    light->direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.1f));

    light->ambient = glm::vec3(0.5f);
    light->diffuse = glm::vec3(0.3f);
    light->specular = glm::vec3(0.9f);

    GLuint whiteSpecular = ResourceManager::CreateTextureFromColor("white_spec", glm::vec3(1.0f));

    RenderHelper::SetSpecularTexture(model1, whiteSpecular);

    //model1->GetComponent<RigidbodyComponent>()->useGravity = true;
    //model1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 1, 1 };
    //model1->GetComponent<TransformComponent>()->position.y = 150;
    //RenderHelper::SetMaterial(model29, brickMat);

    /*
    GameObject* raycastTarget = placeholderModel->Instantiate(*scena1, nullptr, ourShader.get());

    auto* targetTr = raycastTarget->GetComponent<TransformComponent>();
    targetTr->position = glm::vec3(0.0f, 1.0f, -30.0f); // 30 jednostek przed placeholderem
    targetTr->scale = glm::vec3(3.0f);
    targetTr->isDirty = true;

    auto* targetCol = raycastTarget->AddComponent<ColliderComponent>();
    targetCol->halfSize = glm::vec3(3.0f);
    targetCol->offset = glm::vec3(0.0f);
    *//*
    GameObject* RaycastSource =
        CreateRaycastTestObject(
            *scena1,
            *placeholderModel,
            ourShader.get(),
            glm::vec3(0.0f, 5.0f, -20.0f),
            glm::vec3(1.0f)
        );
    NavPathComponent* sourceAgent = RaycastSource->AddComponent<NavPathComponent>();
    RaycastSource->AddComponent<RigidbodyComponent>();
    sourceAgent->moveSpeed = 6.0f;
    sourceAgent->debugDraw = true;*/
    /*
 ustawianiePokoju
    GameObject* model5 = cupModel->Instantiate(*scena1, nullptr, ourShader.get());
    model5->GetComponent<TransformComponent>()->position.x = 0.0f;
    model5->GetComponent<TransformComponent>()->position.y = 2.0f;
    model5->GetComponent<TransformComponent>()->position.z = 20.0f;
    */

    sceneManager.Update(16);

    spdlog::info("Scena git.");

    focused = true;
    updateFocus();

    auto* t0 = camera1->GetComponent<TransformComponent>();
    auto* t1 = camera2->GetComponent<TransformComponent>();

    renderSystem = ecs.GetSystem<RenderSystem>();
    postProcessingSystem = ecs.GetSystem<PostProcessingSystem>();

   createFirstRoom(scena1);

    ecs.GetSystem<NavMeshSystem>()->Bake(*scena1);

    // aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

    dyingModelPrefab = std::make_unique<Prefab>("res/models/Dying.fbx");
    jumpSkeletonPrefab = std::make_unique<Prefab>("res/models/Jump.fbx");

    GameObject* dyingObj = dyingModelPrefab->Instantiate(*scena1, nullptr, ourShader.get());

    dyingObj->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, -50.0f, -50.0f);
    dyingObj->GetComponent<TransformComponent>()->scale = glm::vec3(0.1f);

    AnimatorComponent* animator = dyingObj->AddComponent<AnimatorComponent>();

    // aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

    //PostProcessTest
    RedModel = std::make_unique<Prefab>("res/models/test/red_test.glb");
    GreenModel = std::make_unique<Prefab>("res/models/test/green_test.glb");
    BlueModel = std::make_unique<Prefab>("res/models/test/blue_test.glb");

    GameObject* redObject = RedModel->Instantiate(*scena1, nullptr, ourShader.get());
    GameObject* blueObject = BlueModel->Instantiate(*scena1, nullptr, ourShader.get());
    GameObject* greenObject = GreenModel->Instantiate(*scena1, nullptr, ourShader.get());

    redObject->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 30.0f, 50.0f);
    blueObject->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 30.0f, 0.0f);
    greenObject->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 30.0f, -50.0f);

    rigidBodyCamera1->useGravity = true;
    rigidBodyCamera2->useGravity = true;

    //FMOD
    FMOD::Sound* sound = nullptr;

    ecs.GetSystem<AudioSystem>()->createSound("res/sound/door_unlock.wav", sound);

    //obracanie
    std::unordered_map<GameObject*, float> rotatingObjects;

    auto normalizeAngle = [](float angle) -> float {
        angle = fmod(angle, 360.0f);
        if (angle < 0.0f) angle += 360.0f;
        return angle;
        };

    auto checkKibelUstawienia = [&]() {
        const float expectedAngles[6] = { 0.0f, -60.0f, -180.0f, -120.0f, -240.0f, -300.0f };

        // normalizujemy expected tez bo np -60 -> 300, -180 -> 180 itd
        bool allCorrect = true;
        for (int i = 0; i < 6; i++) {
            TransformComponent* transform = tablicaPapierowKibel[i]->GetComponent<TransformComponent>();
            if (transform == nullptr) { allCorrect = false; continue; }

            float current = normalizeAngle(transform->rotation.z);
            float expected = normalizeAngle(expectedAngles[i]);

            bool correct = fabs(current - expected) < 1.0f; // tolerancja 1 stopien
            spdlog::info("Kibel[{}] rotacja Z: {:.2f} (oczekiwana: {:.2f}) - {}",
                i, current, expected, correct ? "OK" : "ZLE");

            if (!correct) allCorrect = false;
        }

        if (allCorrect == true) {
            ecs.GetSystem<AudioSystem>()->playSound(sound);
        }

        can_open_door_1 = allCorrect;
        };

    //Karaluch center

    glm::vec3 nestPos = glm::vec3(0.0f, 0.5f, -80.0f);

    GameObject* leader = CreateCockroachLeader(*scena1, *placeholderModel, ourShader.get(), nestPos, 4.0f);

    for (int i = 0; i < 3; i++) {
        glm::vec3 offset = glm::vec3(
            (float)(rand()%6) - 3.0f, 0,
            (float)(rand()%6) - 3.0f
        );
        CreateCockroachFollower(
            *scena1, *placeholderModel, ourShader.get(),
            leader, nestPos + offset, 4.5f);
    }



    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        updateFPS(deltaTime);

        CpuTimer cpuTimer;
        cpuTimer.start();

        UpdateDoors(deltaTime);

        // --- CPU WORK START ---

        auto inputStart = std::chrono::high_resolution_clock::now();

        if (ecs.GetSystem<HID>()->is_action_just_pressed("right_click")) {
            focused = !focused;
            updateFocus();
        }
        if (ecs.GetSystem<HID>()->is_action_just_pressed("toggle_frustum_culling")) {
            renderSystem->frustumCullingEnabled = !renderSystem->frustumCullingEnabled;
            spdlog::info("Frustum culling: {}",
                renderSystem->frustumCullingEnabled ? "ON" : "OFF");
        }
        if (ecs.GetSystem<HID>()->is_action_just_pressed("toggle_oclussion_culling")) {
            renderSystem->occlusionCullingEnabled = !renderSystem->occlusionCullingEnabled;
            spdlog::info("Oclussion culling: {}",
                renderSystem->frustumCullingEnabled ? "ON" : "OFF");
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
                if (rotatableObjects.count(hit.hitObject)) {
                    hintText = "Rotate";
                }
                else if (unlockedDoors.count(hit.hitObject)) {
                    hintText = "Open";
                }
                else if (majorDoors.count(hit.hitObject)) {
                    hintText = "Unlock"; //placeholder poki co
                }
                else if (pickupObjects.count(hit.hitObject)) {
                    hintText = (hit.hitObject == p2HeldObject) ? "Held by Player2" : "Pick up";
                }
            }
        }
        player1InteractionInfo->text = hintText;

        std::string hintText2 = "";
        if (player2Raycast->anyHit()) {

            RaycastHit hit = player2Raycast->closestHit();

            if (hit.hitObject != nullptr) {
                if (rotatableObjects.count(hit.hitObject)) {
                    hintText2 = "Rotate";
                }
                else if (unlockedDoors.count(hit.hitObject)) {
                    hintText2 = "Open";
                }
                else if (majorDoors.count(hit.hitObject)) {
                    hintText2 = "Unlock"; //placeholder poki co
                }
                else if (pickupObjects.count(hit.hitObject)) {
                    hintText = (hit.hitObject == p1HeldObject) ? "Held by Player1" : "Pick up";
                }
            }
        }
        player2InteractionInfo->text = hintText2;


        for (auto it = rotatingObjects.begin(); it != rotatingObjects.end(); )
        {
            TransformComponent* transform = it->first->GetComponent<TransformComponent>();
            if (transform == nullptr) { it = rotatingObjects.erase(it); continue; }

            float step = 90.0f * deltaTime;
            if (step > it->second) step = it->second;

            transform->rotation.z -= step;
            it->second -= step;

            if (it->second <= 0.0f)
            {
                spdlog::info("Rotated to: {:.2f}", transform->rotation.z);
                it = rotatingObjects.erase(it);
                checkKibelUstawienia();
            }
            else ++it;
        }


        // caly ten wielki kod wydzielilem do funkcji
        HandlePlayerInteraction(ecs, "interact_p1", player1Raycast, camera1, p1HeldObject, p2HeldObject, scena1, rotatingObjects);
        HandlePlayerInteraction(ecs, "interact_p2", player2Raycast, camera2, p2HeldObject, p1HeldObject, scena1, rotatingObjects);

        // testy animacji

        //animacja umierania (wywoływanie animacji po nazwie z pliku modelu)
        if (ecs.GetSystem<HID>()->is_action_just_pressed("anim_play_dying")) {
            auto* clip = AnimationHelper::FindAnimation(dyingModelPrefab->rootModel->animations, "mixamo.com");
            if (clip) {
                AnimationHelper::Play(animator, clip, true, 1.0f);
                spdlog::info("Odtworzono animacje umierania");
            }
        }

        //animacja skoku (wywoływanie animacji po indeksie - pierwsza z Jump.fbx)
        if (ecs.GetSystem<HID>()->is_action_just_pressed("anim_play_jump")) {
            auto* clip = &jumpSkeletonPrefab->rootModel->animations[0];
            if (clip) {
                AnimationHelper::Play(animator, clip, true, 1.0f);
                spdlog::info("Odtworzono animacje skoku");
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

        // testy animacji

        // Process I/O operations here
        input();

        if (focused)
        {
            processCameraInput(ecs, *camCompLeft, *t0,
                "move_up", "move_down", "move_left", "move_right");

            processCameraInput(ecs, *camCompLeft, *t0,
                "move_up", "move_down", "move_left", "move_right");

            processCameraInput(ecs, *camCompRight, *t1,
                "move_up_2", "move_down_2", "move_left_2", "move_right_2");


            processCameraMouse(ecs, *camCompLeft, *camTransform1);
            processCameraGamepad(ecs, *camCompLeft, *t0, 0);
            processCameraGamepad(ecs, *camCompRight, *t1, 1);

            processCameraMouse(ecs, *camCompLeft, *camTransform1);
            processCameraGamepad(ecs, *camCompLeft, *t0, 0);
            processCameraGamepad(ecs, *camCompRight, *t1, 1);
        }

        auto inputEnd = std::chrono::high_resolution_clock::now();


        auto logicStart = std::chrono::high_resolution_clock::now();
        sceneManager.Update(deltaTime);
        // Update game objects' state here
        update();

        auto logicEnd = std::chrono::high_resolution_clock::now();

        // OpenGL rendering code here
        //render();

        // Draw ImGui
        imgui_begin();
        imgui_render(sceneManager); // edit this function to add your own ImGui controls
        imgui_end(); // this call effectively renders ImGui

        // --- CPU WORK END ---

        cpuTimer.stop();

        float cpuFrameTime = cpuTimer.getMilliseconds();

        float logicTime = std::chrono::duration<float, std::milli>(logicEnd - logicStart).count();
        float inputTime = std::chrono::duration<float, std::milli>(inputEnd - inputStart).count();

        perf.cpuFrameTime = cpuFrameTime;
        perf.logicTime = logicTime;
        perf.inputTime = inputTime;

        // End frame and swap buffers (double buffering)
        end_frame();

    }

    // sound->release();

     // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);

    groundModel.reset();
    sunModel.reset();

    ResourceManager::Clear();

    glfwTerminate();

    return 0;
}

bool init()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        spdlog::error("Failed to initalize GLFW!");
        return false;
    }

    // GL 4.6 + GLSL 460
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MimiCry", NULL, NULL);
    if (window == NULL)
    {
        spdlog::error("Failed to create GLFW Window!");
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync - fixes FPS at the refresh rate of your screen
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);


    bool err = !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    if (err)
    {
        spdlog::error("Failed to initialize OpenGL loader!");
        return false;
    }
    return true;
}

void init_imgui()
{
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
}


void compileShader()
{


    spdlog::info("Success");

}

void input()
{
//OLD INPUT STARTS HERE

    // I/O ops go here
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}


void update()
{
    // Update game objects' state here
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    //if (mouseMove)
    //    CameraHelper::ProcessMouseMovement(cam, xoffset, yoffset);
}
//
//// glfw: whenever the mouse scroll wheel scrolls, this callback is called
//// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void imgui_begin()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

GameObject* selectedGameObject = nullptr;

void ShowGameObjectTree(GameObject* obj)
{
    if (!obj)
        return;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ((obj == selectedGameObject) ? ImGuiTreeNodeFlags_Selected : 0);

    const char* displayName = obj->name.empty() ? "GameObject" : obj->name.c_str();

    // jeśli brak dzieci -> leaf
    if (!obj->HasChildren())
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool opened = ImGui::TreeNodeEx((void*)obj, flags, "%s", displayName);

    if (ImGui::IsItemClicked())
        selectedGameObject = obj;

    if (opened)
    {
        for (GameObject* child : obj->GetChildren())
        {
            ShowGameObjectTree(child);
        }

        ImGui::TreePop();
    }
}

void ShowTransformEditor(TransformComponent& transform)
{
    glm::vec3 pos = TransformHelper::getLocalPosition(transform);
    glm::vec3 rot = TransformHelper::getLocalRotation(transform);
    glm::vec3 scale = TransformHelper::getLocalScale(transform);

    if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
    {
        TransformHelper::setLocalPosition(transform, pos);
    }

    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
    {
        TransformHelper::setLocalRotation(transform, rot);
    }

    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.01f))
    {
        TransformHelper::setLocalScale(transform, scale);
    }
}

void ShowColliderEditor(ColliderComponent& collider)
{
    ImGui::Checkbox("Is Trigger", &collider.isTrigger);

    //DrawVec3Control(
    //    "Offset",
    //    collider.offset
    //);

    //DrawVec3Control(
    //    "Half Size",
    //    collider.halfSize,
    //    0.01f,
    //    0.5f
    //);
}
//
//void DrawVec3Control(
//    const std::string& label,
//    glm::vec3& values,
//    float speed = 0.01f,
//    float resetValue = 0.0f)
//{
//    ImGui::PushID(label.c_str());
//
//    ImGui::Columns(2);
//    ImGui::SetColumnWidth(0, 100.0f);
//    ImGui::Text(label.c_str());
//    ImGui::NextColumn();
//
//    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
//
//    float lineHeight =
//        ImGui::GetFontSize() +
//        ImGui::GetStyle().FramePadding.y * 2.0f;
//
//    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };
//
//    // X
//    ImGui::PushStyleColor(ImGuiCol_Button,
//        ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
//
//    if (ImGui::Button("X", buttonSize))
//        values.x = resetValue;
//
//    ImGui::SameLine();
//
//    ImGui::DragFloat("##X", &values.x, speed);
//
//    ImGui::PopItemWidth();
//    ImGui::SameLine();
//
//    ImGui::PopStyleColor();
//
//    // Y
//    ImGui::PushStyleColor(ImGuiCol_Button,
//        ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
//
//    if (ImGui::Button("Y", buttonSize))
//        values.y = resetValue;
//
//    ImGui::SameLine();
//
//    ImGui::DragFloat("##Y", &values.y, speed);
//
//    ImGui::PopItemWidth();
//    ImGui::SameLine();
//
//    ImGui::PopStyleColor();
//
//    // Z
//    ImGui::PushStyleColor(ImGuiCol_Button,
//        ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
//
//    if (ImGui::Button("Z", buttonSize))
//        values.z = resetValue;
//
//    ImGui::SameLine();
//
//    ImGui::DragFloat("##Z", &values.z, speed);
//
//    ImGui::PopItemWidth();
//
//    ImGui::PopStyleColor();
//
//    ImGui::Columns(1);
//
//    ImGui::PopID();
//}



void ShowLightEditor(LightComponent& light)
{
    // enable
    ImGui::Checkbox("Enabled", &light.isOn);

    // type
    const char* lightTypes[] =
    {
        "Directional",
        "Point",
        "Spot"
    };

    int currentType = static_cast<int>(light.type);

    if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
    {
        light.type = static_cast<LightType>(currentType);
    }

    ImGui::Separator();

    ImGui::ColorEdit3("Ambient", &light.ambient.x);
    ImGui::ColorEdit3("Diffuse", &light.diffuse.x);
    ImGui::ColorEdit3("Specular", &light.specular.x);

    ImGui::Separator();

    // direction
    //if (light.type == Directional || light.type == Spot)
    //{
    //    ImGui::DragFloat3("Direction", &light.direction.x, 0.01f);
    //}

    // attenuation
    if (light.type == Point || light.type == Spot)
    {
        ImGui::Text("Attenuation");
        ImGui::DragFloat("Constant", &light.constant, 0.001f, 0.0f, 10.0f);
        ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 10.0f);
        ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 10.0f);
    }

    // spotlight
    if (light.type == Spot)
    {
        ImGui::Separator();
        ImGui::Text("Spotlight");

        float innerAngle = glm::degrees(glm::acos(light.cutOff));
        float outerAngle = glm::degrees(glm::acos(light.outerCutOff));

        if (ImGui::DragFloat("Inner Cutoff", &innerAngle, 0.1f, 0.0f, 90.0f))
        {
            light.cutOff = glm::cos(glm::radians(innerAngle));
        }

        if (ImGui::DragFloat("Outer Cutoff", &outerAngle, 0.1f, 0.0f, 90.0f))
        {
            light.outerCutOff = glm::cos(glm::radians(outerAngle));
        }
    }
}

std::string OpenFileDialog()
{
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;

    ofn.lpstrInitialDir = "res";

    ofn.lpstrFilter =
        "Model Files\0*.obj;*.fbx;*.glb;*.gltf\0"
        "All Files\0*.*\0";

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
    {
        std::filesystem::path fullPath = filename;

        std::filesystem::path projectRoot = std::filesystem::absolute("../../");

        std::filesystem::path relative =
            std::filesystem::relative(fullPath, projectRoot);

        std::string result = relative.string();

        std::replace(result.begin(), result.end(), '\\', '/');

        return result;
    }

    return "";
}

static std::unordered_map<std::string, Prefab> prefabs;

void imgui_render(SceneManager& sceneManager)
{
    if (show_demo_window)
    {
        //ImGui::ShowDemoWindow(&show_demo_window);
    }

    ImGui::Begin("Hello, world!");

    if (ImGui::Button(wireframeMode ? "Switch to Fill Mode" : "Switch to Wireframe"))
    {
        wireframeMode = !wireframeMode;
        glPolygonMode(GL_FRONT_AND_BACK,
            wireframeMode ? GL_LINE : GL_FILL);
    }

    ImGui::Separator();
    ImGui::Text("Hierarchy");
    //entityFilter.Draw("Search", 200);
    ShowGameObjectTree(sceneManager.GetActiveScene()->GetRoot());

    if (selectedGameObject)
    {
        ImGui::Separator();
        ImGui::Text("Selected Entity: %s", selectedGameObject->name.c_str());
        ShowTransformEditor(*selectedGameObject->GetComponent<TransformComponent>());

        LightComponent* light = selectedGameObject->GetComponent<LightComponent>();

        if (light != nullptr)
        {
            ShowLightEditor(*light);
        }
    }

    if (ImGui::Button("Zapisz"))
    {
        sceneManager.Save();
    }

    ImGui::Separator();

    Scene& scene = *sceneManager.GetActiveScene();

    for (auto& [name, weakModel] : ResourceManager::Models)
    {
        ImGui::PushID(name.c_str());

        std::shared_ptr<Model> model = weakModel;

        ImGui::Text("%s", name.c_str());
        ImGui::SameLine();

        if (!model)
        {
            ImGui::TextDisabled("[loading]");
        }
        else
        {
            if (!prefabs.contains(name))
            {
                prefabs.emplace(name, Prefab(model));
            }

            if (ImGui::Button("Instantiate"))
            {
                Prefab& prefab = prefabs.at(name);

                GameObject* obj =
                    prefab.Instantiate(scene, nullptr, ourShader.get());

                if (obj)
                    obj->name = name;
            }
        }

        ImGui::PopID();
    }


    ImGui::End();

    ImGui::Begin("Loaded Models");

    if (ImGui::Button("Load Model"))
    {
        std::string path = OpenFileDialog();

        if (!path.empty())
        {
            ResourceManager::LoadModel(path);
        }
    }


    if (ImGui::Button("Load asset"))
    {
        std::string path = "assets.yaml";
        ResourceManager::LoadAssets(path);
    }

    if (ImGui::Button("Zapisz asset"))
    {
        ResourceManager::SaveAsset();
    }

    ImGui::Separator();


    for (const auto& [name, weakModel] : ResourceManager::Models)
    {
        ImGui::Text("%s", name.c_str());
    }

    ImGui::End();

    ImGui::Begin("Performance");

    // FPS i frame time
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    // CPU breakdown
    if (ImGui::CollapsingHeader("CPU")) {
        ImGui::Text("Total CPU: %.3f ms", perf.cpuFrameTime);
        ImGui::Text("Input:     %.3f ms", perf.inputTime);
        ImGui::Text("Logic:     %.3f ms", perf.logicTime);
        ImGui::Text("Culling:   %.3f ms", renderSystem->stats.cullingTimeMs);
        ImGui::Text("Draw prep: %.3f ms", renderSystem->stats.drawSubmitTimeMs);
    }
    // GPU
    if (ImGui::CollapsingHeader("GPU")) {
        ImGui::Text("GPU Frame: %.3f ms", renderSystem->gpuQuery.getLastResult());
    }

    if (ImGui::CollapsingHeader("Render Stats")) {
        ImGui::Text("Draw calls:   %d", renderSystem->stats.drawCalls);
        ImGui::Text("Objects:      %d", renderSystem->stats.renderedObjects);
        ImGui::Text("Triangles:    %d", renderSystem->stats.triangles);
        ImGui::Text("State changes:%d", renderSystem->stats.stateChanges);
    }
    if (ImGui::CollapsingHeader("Culling")) {
        ImGui::Checkbox("Frustum culling", &renderSystem->frustumCullingEnabled);
        ImGui::Checkbox("Occlusion culling", &renderSystem->occlusionCullingEnabled);
        ImGui::Text("Frustum culled:   %d", renderSystem->stats.frustumCulledSet.size());
        ImGui::Text("Occlusion culled: %d", renderSystem->stats.occlusionCulledSet.size());
    }
    ImGui::PlotLines("Frame time", frameTimes, MAX_SAMPLES, index,
        nullptr, 0.0f, 1.0f, ImVec2(0, 60));

    ImGui::End();

    /*ImGui::SliderFloat("rotation X", &rotationX, -480.0f, 480.0f);
    ImGui::SliderFloat("rotation Y", &rotationY, -480.0f, 480.0f);
    ImGui::SliderFloat("Camera Distance", &cameraDistance, 5.0f, 1000.0f);*/
}

void imgui_end()
{
    ImGui::Render();
    int display_w, display_h;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &display_w, &display_h);

    glViewport(0, 0, display_w, display_h);


    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void end_frame()
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}


void processCameraGamepad(ECS &ecs, CameraComponent &cam, TransformComponent &transform, int gamepad_id) {
    const auto& hid = ecs.GetSystem<HID>();

    // ruch
    float lx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_X, gamepad_id);
    float ly = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_Y, gamepad_id);

    glm::vec3 dir(0.0f);

    glm::vec3 camFront = cam.state.Front;
    camFront.y = 0.0f;
    camFront = glm::normalize(camFront);

    glm::vec3 camRight = cam.state.Right;
    camRight.y = 0.0f;
    camRight = glm::normalize(camRight);

    dir += camFront * (-ly); // przód/tył
    dir += camRight * lx;    // lewo/prawo

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        transform.position += dir * MovementSpeed * deltaTime;
    }

    // obrót kamery
    float rx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_X, gamepad_id);
    float ry = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_Y, gamepad_id);

    const float sensitivity = 600.0f;

    CameraHelper::ProcessMouseMovement(cam, transform,
        rx * sensitivity * deltaTime,
        ry * sensitivity * deltaTime
    );
}

void addAllSystems(ECS &ecs) {
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
void connectAllModels() {
    bed1Model = std::make_unique<Prefab>("res/models/samochod.glb");
    bed2Model = std::make_unique<Prefab>("res/models/bed2.glb");
    bed3Model = std::make_unique<Prefab>("res/models/bed3.glb");
    corkBoardModel = std::make_unique<Prefab>("res/models/cork_board.glb");
    cupModel = std::make_unique<Prefab>("res/models/cup.glb");
    placeholderModel = std::make_unique<Prefab>("res/models/placeholder.glb");
    //deskModel      = std::make_unique<Prefab>("res/models/desk.glb");
    //doorsModel     = std::make_unique<Prefab>("res/models/doors.glb");
    //folderModel    = std::make_unique<Prefab>("res/models/folder.glb");
    //krzesloModel   = std::make_unique<Prefab>("res/models/krzeslo.glb");
    ////ksiazkaModel   = std::make_unique<Prefab>("res/models/ksiazka.glb");
    //lampa1Model    = std::make_unique<Prefab>("res/models/lampa1.glb");
    //lampa2Model    = std::make_unique<Prefab>("res/models/lampa2.glb");
    //lampa3Model    = std::make_unique<Prefab>("res/models/lampa3.glb");
    //needleModel    = std::make_unique<Prefab>("res/models/needle.glb");
    //bad1Model      = std::make_unique<Prefab>("res/models/obiekty_bad1.glb");
    //bad2Model      = std::make_unique<Prefab>("res/models/obiekty_bad2.glb");
    //bad3Model      = std::make_unique<Prefab>("res/models/obiekty_bad3.glb");
    //papersModel    = std::make_unique<Prefab>("res/models/papers.glb");
    //bossModel      = std::make_unique<Prefab>("res/models/placeholder_boss.glb");
    //characterModel = std::make_unique<Prefab>("res/models/placeholder_character.glb");
    //vial1Model     = std::make_unique<Prefab>("res/models/probowka1.glb");
    //vial2Model     = std::make_unique<Prefab>("res/models/probowka2.glb");
    //vial3Model     = std::make_unique<Prefab>("res/models/probowka3.glb");
    //vial4Model     = std::make_unique<Prefab>("res/models/probowka4.glb");
    //vial5Model     = std::make_unique<Prefab>("res/models/probowka5.glb");
    //vial61Model    = std::make_unique<Prefab>("res/models/probowka6.glb");
    //vial7Model     = std::make_unique<Prefab>("res/models/probowka7.glb");
    sinkModel      = std::make_unique<Prefab>("res/models/sink.glb");
    //szafa1Model    = std::make_unique<Prefab>("res/models/szafa1.glb");
    //szafa2Model    = std::make_unique<Prefab>("res/models/szafa2.glb");
    //szafa3Model    = std::make_unique<Prefab>("res/models/szafa3.glb");
    //telephoneModel = std::make_unique<Prefab>("res/models/telephone.glb");
    toiletModel    = std::make_unique<Prefab>("res/models/toilet.glb");
    doorsToiletModel    = std::make_unique<Prefab>("res/models/doors_toilet.glb");
    toiletPaperModel = std::make_unique<Prefab>("res/models/toilet_paper_mystery_2.glb");
    mirrorModel1 = std::make_unique<Prefab>("res/models/glass1.glb");
    mirrorModel2 = std::make_unique<Prefab>("res/models/glass2.glb");
    mirrorModel3 = std::make_unique<Prefab>("res/models/glass3.glb");
    washroomExit = std::make_unique<Prefab>("res/models/door_other_2.glb");
    //wozekModel     = std::make_unique<Prefab>("res/models/wozek.glb");
    //zaslonaModel   = std::make_unique<Prefab>("res/models/zaslona.glb");
    //roomModel = std::make_unique<Prefab>("res/models/room.glb");
}