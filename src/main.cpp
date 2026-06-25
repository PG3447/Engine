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

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <model.h>
#include <prefab.h>
#include <filesystem>
#include <optional>
#include <fstream>
#include <sstream>

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
#include "systems/UI_system.h"
#include "utils/render_helper.h"
#include "utils/animation_helper.h"
#include "utils/player_animation_helper.h"
#include "systems/NavMeshBenchmark.h"

#include "gameplay/crematorium_puzzle.h"
#include "gameplay/menu.h"
#include "systems/SurfaceDecorationSystem.h"
#include "utils/transform_gizmo.h"
#include "impl/main/room.h"
#include "utils/placement_editor.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool init();
void init_imgui();

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

bool processCameraGamepad(ECS& ecs, CameraComponent& cam, TransformComponent& transformCamera, TransformComponent& playerTransform, int gamepad_id, bool& outIsTurning);
void connectAllModels();

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
SurfaceDecorationSystem decorSystem;

GLuint VBO;
GLuint VAO;
GLuint texture;
std::unique_ptr<Shader> skyboxShader;
extern GameObject * selectedGameObject;
//std::unique_ptr<Shader> reflectShader;
//std::unique_ptr<Shader> refractShader;

#include "impl/main/list_prefab.ipp"

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

    glm::vec3 closedHalfSize = glm::vec3(0.8f, 10.0f, 4.0f);
    glm::vec3 openHalfSize   = glm::vec3(1.0f, 10.0f, 1.0f);
    glm::vec3 openOffset     = glm::vec3(0.0f);
    bool requiresUnlock = false;
};

std::unordered_map<GameObject*, CabinetState> cabinetsMap;
std::unordered_map<GameObject*, DoorState>    toiletDoorsMap;
std::unordered_set<GameObject*>               pickupObjects;
GameObject* p1HeldObject = nullptr;
GameObject* p2HeldObject = nullptr;

bool p1IsReading = false;
bool p2IsReading = false;

GameObject* p1NoteUI_obj = nullptr;
SpriteComponent* p1NoteUI = nullptr;

GameObject* p2NoteUI_obj = nullptr;
SpriteComponent* p2NoteUI = nullptr;

std::unordered_map<GameObject*, std::string> noteContents;

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

#include "impl/main/process_input.ipp"

void LoadPlayerAnimations(Prefab* postacGracza);
void createRentgenCorridor(Scene * scena);
void createCrematoriumCorridor(Scene * scena);

#include "impl/main/logic_update.ipp"

void createFirstRoom(Scene* scena1);
void createMainRooom(Scene* scena);
void createNuclearRooom(Scene* scena);
void createCrematorium(Scene* scena);
void createRentgenRoom(Scene* scena);
void createTrigger(Scene* scena);
void CheckFallenPickupObjects();


void loadGame()
{
    
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

    ECS* ecs;
    SceneManager sceneManager(ecs, renderSystem, postProcessingSystem, window);

    sceneManager.CreateScene("Scena 1");
    sceneManager.CreateScene("menu");
    sceneManager.CreateScene("loading");

    Scene* scena1 = sceneManager.GetActiveScene();
    Scene* scenaMenu = sceneManager.GetScene("menu");
    Scene* scenaLoading = sceneManager.GetScene("loading");
    ecs = &scena1->GetECS();

    scenaLoading->GetECS().AddSystem<TransformSystem>(scenaLoading->GetECS());
    scenaLoading->GetECS().AddSystem<SpriteSystem>(scenaLoading->GetECS(), window);
    scenaMenu->GetECS().AddSystem<HID>(scenaMenu->GetECS(), window);
    scenaMenu->GetECS().AddSystem<UISystem>(scenaMenu->GetECS(), *scenaMenu->GetECS().GetSystem<HID>());
    scenaMenu->GetECS().AddSystem<TransformSystem>(scenaMenu->GetECS());
    scenaMenu->GetECS().AddSystem<RenderSystem>(scenaMenu->GetECS(), window);
    scenaMenu->GetECS().AddSystem<SpriteSystem>(scenaMenu->GetECS(), window);
    scenaMenu->GetECS().AddSystem<PostProcessingSystem>(scenaMenu->GetECS(), window);
    scenaMenu->GetECS().GetSystem<PostProcessingSystem>()->SetActive(false);
    scena1->addAllSystems(window);
    

    GameObject* loadingObject = scenaLoading->CreateGameObject(nullptr);
    SpriteComponent* loadingSprite = loadingObject->AddComponent<SpriteComponent>();
    loadingSprite->sprites = { ResourceManager::LoadTexture("LOADING.png", "res/sprites/").id };
    loadingSprite->screenPosition = glm::vec2(0.0f, 0.0f);
    loadingSprite->size = glm::vec2(1920.0f, 1080.0f);
    loadingSprite->layer = 1;
    loadingSprite->isVisible = true;


    sceneManager.ChangeScene("loading");
    sceneManager.UpdateChangeScene();
    sceneManager.Update(0.16f);
    end_frame();


    Menu menu(&sceneManager, scenaMenu, window);
    menu.Init();

    sceneManager.ChangeScene("Scena 1");
    sceneManager.UpdateChangeScene();
    //menu->GetECS().AddExistingSystem(scena1->GetECS().GetSystem<RenderSystem>());

    postacGraczaCzerw = std::make_unique<Prefab>("res/models/postac_akcje_czerw.glb");
    postacGraczaZiel = std::make_unique<Prefab>("res/models/postac_akcje_ziel.glb");
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

    //GameObject * lightComponent = scena1->CreateGameObject(nullptr);
    //lightComponent->name = "light";
    //lightComponent->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 20.0f, 0.0f);
    //lightComponent->GetComponent<TransformComponent>()->rotation = glm::vec3(-19.800, -7.300, 0.0f);
    //lightComponent->AddComponent<LightComponent>();
    //LightComponent * lc = lightComponent->GetComponent<LightComponent>();
    //lc->type = LightType::Directional;
    //lc->index     = 20;
    //lc->constant  = 1.0f;
    //lc->linear    = 0.10f;
    //lc->quadratic = 0.00001f;
    //lc->intensity = 2.000;

    //Tworzenie gracza nr.1
    GameObject* gracz1 = scena1->CreateGameObject(nullptr);
    gracz1->name = "Gracz1";

    ColliderComponent* camera1collider = gracz1->AddComponent<ColliderComponent>();
    RigidbodyComponent* rigidBodyCamera1 = gracz1->AddComponent<RigidbodyComponent>();
    gracz1->GetComponent<TransformComponent>()->position = glm::vec3(11.986f, 6.250f, -12.000f);
    gracz1->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    gracz1->GetComponent<RigidbodyComponent>()->mass = 10.0f;
    gracz1->GetComponent<RigidbodyComponent>()->bounce = 0.1f;
    gracz1->GetComponent<RigidbodyComponent>()->useGravity = true;
    gracz1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.2f, 5.25f, 1.2f };



    GameObject* camera1 = scena1->CreateGameObject(nullptr);//groundModel->Instantiate(*scena1, nullptr, ourShader.get());
    camera1->name = "Kamera";
    gracz1->AddChild(camera1);
    camera1->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 4.7f, -0.8f);
    CameraComponent* camCompLeft = camera1->AddComponent<CameraComponent>();
    RaycastComponent*  player1Raycast   = camera1->AddComponent<RaycastComponent>();
    player1Raycast->debugDraw = false;

    GameObject* latarka1 = scena1->CreateGameObject(nullptr);
    latarka1->name = "Latarka";
    camera1->AddChild(latarka1);
    latarka1->GetComponent<TransformComponent>()->position = glm::vec3(1.10f, -0.11f, -0.25f);
    latarka1->GetComponent<TransformComponent>()->rotation = glm::vec3(-0.40f, 4.50f, 0.00f);
    LightComponent* light2 = latarka1->AddComponent<LightComponent>();

    light2->type      = Spot;
    light2->index     = 0;
    light2->ambient   = glm::vec3(0.25f);
    light2->diffuse   = glm::vec3(1.0f);
    light2->specular  = glm::vec3(1.0f);
    light2->constant  = 1.0f;
    light2->linear    = 0.10f;
    light2->quadratic = 0.00001f;
    light2->intensity = 650.0f;
    light2->cutOff      = glm::cos(glm::radians(8.0f));
    light2->outerCutOff = glm::cos(glm::radians(22.0f));

    GameObject* modelPostac1 = postacGraczaCzerw->Instantiate(*scena1, nullptr, nullptr);
    gracz1->AddChild(modelPostac1);
    modelPostac1->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, -0.2f, 0.0f);
    modelPostac1->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
    AnimatorComponent* p1Animator = modelPostac1->GetComponent<AnimatorComponent>();
    if (p1Animator == nullptr) {
        p1Animator = modelPostac1->AddComponent<AnimatorComponent>();
        p1Animator->currentSkeleton = &postacGraczaCzerw->rootModel->skeleton;
    }

    //Tworzenie gracza nr.2
    GameObject* gracz2 = scena1->CreateGameObject(nullptr);
    gracz2->name = "Gracz2";

    ColliderComponent*  camera2collider  = gracz2->AddComponent<ColliderComponent>();
    RigidbodyComponent* rigidBodyCamera2 = gracz2->AddComponent<RigidbodyComponent>();
    gracz2->GetComponent<TransformComponent>()->position = glm::vec3(0.070, 6.250f, -18.649f);
    gracz2->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    gracz2->GetComponent<RigidbodyComponent>()->mass = 10.0f;
    gracz2->GetComponent<RigidbodyComponent>()->bounce = 0.1f;
    gracz2->GetComponent<RigidbodyComponent>()->useGravity = true;
    gracz2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.2f, 5.25f, 1.2f };

    GameObject* camera2 = scena1->CreateGameObject(nullptr);
    camera2->name = "Kamera";
    gracz2->AddChild(camera2);
    camera2->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 4.7f, -0.8f);
    CameraComponent* camCompRight = camera2->AddComponent<CameraComponent>();
    RaycastComponent* player2Raycast = camera2->AddComponent<RaycastComponent>();
    player2Raycast->debugDraw = false;

    GameObject* latarka2 = scena1->CreateGameObject(nullptr);
    latarka2->name = "Latarka";
    camera2->AddChild(latarka2);
    latarka2->GetComponent<TransformComponent>()->position = glm::vec3(1.10f, -0.11f, -0.25f);
    latarka2->GetComponent<TransformComponent>()->rotation = glm::vec3(-0.40f, 4.50f, 0.00f);
    LightComponent* light3 = latarka2->AddComponent<LightComponent>();

    light3->type = Spot;
    light3->index = 2;
    light3->ambient = glm::vec3(0.25f);
    light3->diffuse = glm::vec3(1.0f);
    light3->specular = glm::vec3(1.0f);
    light3->constant = 1.0f;
    light3->linear = 0.10f;
    light3->quadratic = 0.00001f;
    light3->intensity = 650.0f;
    light3->cutOff = glm::cos(glm::radians(8.0f));
    light3->outerCutOff = glm::cos(glm::radians(22.0f));

    GameObject* modelPostac2 = postacGraczaZiel->Instantiate(*scena1, nullptr, nullptr);
    gracz2->AddChild(modelPostac2);
    modelPostac2->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, -0.2f, 0.0f);
    modelPostac2->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
    AnimatorComponent* p2Animator = modelPostac2->GetComponent<AnimatorComponent>();
    if (p2Animator == nullptr) {
        p2Animator = modelPostac2->AddComponent<AnimatorComponent>();
        p2Animator->currentSkeleton = &postacGraczaZiel->rootModel->skeleton;
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

    // UI notatki gracz 1
    p1NoteUI_obj = scena1->CreateGameObject(nullptr);
    p1NoteUI = p1NoteUI_obj->AddComponent<SpriteComponent>();
    p1NoteUI->sprites = { ResourceManager::LoadTexture("note_bg.png", "res/sprites/").id };
    p1NoteUI->size = glm::vec2(500.0f, 700.0f);
    p1NoteUI->screenPosition = glm::vec2(480.0f - 250.0f, 540.0f - 350.0f);
    p1NoteUI->layer = 3;
    p1NoteUI->isVisible = false;

    p1NoteUI->textEnabled = true;
    p1NoteUI->textCentered = false;
    p1NoteUI->textColor = glm::vec3(0.1f, 0.1f, 0.1f);
    p1NoteUI->textOutlineEnabled = false;
    p1NoteUI->fontSize = 14.0f;
    p1NoteUI->textOffset = glm::vec2(30.0f, 30.0f);
    p1NoteUI->text = "";
    p1NoteUI->fontPath = "res/fonts/NothingYouCouldDo-Regular.ttf";


    /// UI notatki gracz 2
    p2NoteUI_obj = scena1->CreateGameObject(nullptr);
    p2NoteUI = p2NoteUI_obj->AddComponent<SpriteComponent>();
    p2NoteUI->sprites = { ResourceManager::LoadTexture("note_bg.png", "res/sprites/").id };
    p2NoteUI->size = glm::vec2(500.0f, 700.0f);
    p2NoteUI->screenPosition = glm::vec2(1440.0f - 250.0f, 540.0f - 350.0f);
    p2NoteUI->layer = 3;
    p2NoteUI->isVisible = false;

    p2NoteUI->textEnabled = true;
    p2NoteUI->textCentered = false;
    p2NoteUI->textColor = glm::vec3(0.1f, 0.1f, 0.1f);
    p2NoteUI->textOutlineEnabled = false;
    p2NoteUI->fontSize = 14.0f;
    p2NoteUI->textOffset = glm::vec2(30.0f, 30.0f);
    p2NoteUI->text = "";
    p1NoteUI->fontPath = "res/fonts/NothingYouCouldDo-Regular.ttf";

    connectAllModels();
    std::string pathAssets = "res/Yaml/assets.yaml";
    ResourceManager::LoadAssets(pathAssets);
    PlacementEditor::LoadPlacements(*scena1, PlacementEditor::DefaultPrefabLookup);
    LoadPlayerAnimations(postacGraczaCzerw.get());
    LoadPlayerAnimations(postacGraczaZiel.get());

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


    renderSystem         = ecs->GetSystem<RenderSystem>();
    postProcessingSystem = ecs->GetSystem<PostProcessingSystem>();

    createFirstRoom(scena1);
    createMainRooom(scena1);
    createNuclearRooom(scena1);
    createCrematorium(scena1);
    createRentgenRoom(scena1);
    createRentgenCorridor(scena1);
    createCrematoriumCorridor(scena1);
    createTrigger(scena1);

    scena1->GetECS().GetSystem<NavMeshSystem>()->BakeRecast(*scena1);
    //dyingModelPrefab   = std::make_unique<Prefab>("res/models/Dying.fbx");
    //jumpSkeletonPrefab = std::make_unique<Prefab>("res/models/Jump.fbx");

    //GameObject* dyingObj = dyingModelPrefab->Instantiate(*scena1, nullptr, nullptr);
    //dyingObj->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, -50.0f, -50.0f);
    //dyingObj->GetComponent<TransformComponent>()->scale    = glm::vec3(0.1f);

    //AnimatorComponent* animator = dyingObj->AddComponent<AnimatorComponent>();

    // PostProcessTest
    //RedModel   = std::make_unique<Prefab>("res/models/test/red_test.glb");
    //GreenModel = std::make_unique<Prefab>("res/models/test/green_test.glb");
    //BlueModel  = std::make_unique<Prefab>("res/models/test/blue_test.glb");

    //GameObject* redObject   = RedModel->Instantiate(*scena1,   nullptr, nullptr);
    //GameObject* blueObject  = BlueModel->Instantiate(*scena1,  nullptr, nullptr);
    //GameObject* greenObject = GreenModel->Instantiate(*scena1, nullptr, nullptr);

    //redObject->GetComponent<TransformComponent>()->position   = glm::vec3(0.0f, 30.0f,  50.0f);
    //blueObject->GetComponent<TransformComponent>()->position  = glm::vec3(0.0f, 30.0f,   0.0f);
    //greenObject->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 30.0f, -50.0f);

    rigidBodyCamera1->useGravity = true;
    rigidBodyCamera2->useGravity = true;

    // FMOD
    AudioSystem* audioSys = ecs->GetSystem<AudioSystem>();

    FMOD::Sound* sound = nullptr;
    audioSys->createSound("res/sound/door_unlock.wav", sound);

    FMOD::Sound* sndAmbient = nullptr;
    audioSys->createSound("res/sound/ambient.aiff", sndAmbient, true);

    if (audioSys && sndAmbient) {
        FMOD::Channel* ambientChannel = audioSys->playSoundEx(sndAmbient);
        //if (ambientChannel) {
        //    ambientChannel->setVolume(0.4f);
        //}
    }

    FMOD::Sound* sndBtnClick = nullptr;
    FMOD::Sound* sndPickup = nullptr;
    FMOD::Sound* sndInsert = nullptr;
    FMOD::Sound* sndDoorLocked = nullptr;
    FMOD::Sound* sndGear = nullptr;
    FMOD::Sound* sndLorePaper = nullptr;
    audioSys->createSound("res/sound/button_click.wav", sndBtnClick, false);
    audioSys->createSound("res/sound/pick_up.wav", sndPickup, false);
    audioSys->createSound("res/sound/insert_puzzle.wav", sndInsert, false);
    audioSys->createSound("res/sound/door_locked.wav", sndDoorLocked, false);
    audioSys->createSound("res/sound/falling_gear.wav", sndGear, false);
    audioSys->createSound("res/sound/page.wav", sndLorePaper, false);

    FMOD::Sound* sndPaperRoll = nullptr;
    FMOD::Sound* sndDoorOpen = nullptr;
    FMOD::Sound* sndDoorClosed = nullptr;
    FMOD::Sound* sndDoorCloseStart = nullptr;

    audioSys->createSound("res/sound/paper_roll.wav", sndPaperRoll, false);
    audioSys->createSound("res/sound/bathroom_door_open.wav", sndDoorOpen, false);
    audioSys->createSound("res/sound/bathroom_door_closed.wav", sndDoorClosed, false);
    audioSys->createSound("res/sound/door_close_start.wav", sndDoorCloseStart, false);

    FMOD::Sound* sndCoffinSlideOut = nullptr;
    FMOD::Sound* sndCoffinSlideIn = nullptr;
    FMOD::Sound* sndCoffinCollide = nullptr;
    FMOD::Sound* sndCoffinClose = nullptr;

    audioSys->createSound("res/sound/coffin_open.wav", sndCoffinSlideOut, true);
    audioSys->createSound("res/sound/coffin_open.wav", sndCoffinSlideIn, true);
    audioSys->createSound("res/sound/coffin_collision.wav", sndCoffinCollide, false);
    audioSys->createSound("res/sound/coffin_closed.wav", sndCoffinClose, false);

    crematoriumPuzzle.SetupAudio(audioSys, sndCoffinSlideOut, sndCoffinSlideIn, sndCoffinCollide, sndCoffinClose, sndGear);

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
            spdlog::info("Kibel[{}] rotacja Z: {:.2f} (oczekiwana: {:.2f}) - {}",
                i, current, expected, correct ? "OK" : "ZLE");

            if (!correct) allCorrect = false;
        }

        if (allCorrect == true) {
            ecs->GetSystem<AudioSystem>()->playSound(sound);
        }

        can_open_door_1 = allCorrect;
    };

    // Karaluch center
    glm::vec3 nestPos = glm::vec3(0.0f, 1.5f, -80.0f);

    //I LOVE THE TASTE OF IRON
    GameObject* Kurorushi = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, nestPos, 4.0f);
    GameObject* KurorushiM = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(5.607, 1.5f, -30.864), 4.0f);
    GameObject* KurorushiM2 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(20.499, 1.5, -50.071), 4.0f);

    GameObject* KurorushiMR1 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(7.070, 1.5f, -125.580), 4.0f);
    GameObject* KurorushiMR2 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(8.502, 1.5f, -168.933), 4.0f);
    GameObject* KurorushiMR3 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(38.413, 1.5f, -115.418), 4.0f);

    GameObject* KurorushiR1 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(-82.576, 1.5f, -185.547), 4.0f);
    GameObject* KurorushiR2 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(-52.370, 1.5f, -157.365), 4.0f);

    GameObject* KurorushiC1 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(150.490, 1.5f, -169.563), 4.0f);
    GameObject* KurorushiC2 = CreateCockroachLeader(*scena1, *cockroachModel, nullptr, glm::vec3(135.006, 1.5f, -140.760), 4.0f);
    //I LOVE THE TASTE OF IRON

    auto spawnFollowers = [&](GameObject* leader, const glm::vec3& pos, int count = 5) {
        for (int i = 0; i < count; i++) {
            glm::vec3 offset = glm::vec3(
                (float)(rand() % 6) - 3.0f, 0,
                (float)(rand() % 6) - 3.0f
            );
            CreateCockroachFollower(
                *scena1, *cockroachModel, nullptr,
                leader, pos + offset, 4.5f);
        }
    };
    spawnFollowers(Kurorushi,   nestPos);
    spawnFollowers(KurorushiM,  glm::vec3(5.607f,  1.5f, -30.864f));
    spawnFollowers(KurorushiM2, glm::vec3(20.499f, 1.5f, -50.071f));
    spawnFollowers(KurorushiMR1, glm::vec3(7.070f,  1.5f, -125.580f));
    spawnFollowers(KurorushiMR2, glm::vec3(8.502f,  1.5f, -168.933f));
    spawnFollowers(KurorushiMR3, glm::vec3(38.413f, 1.5f, -115.418f));
    spawnFollowers(KurorushiR1,  glm::vec3(-82.576f, 1.5f, -185.547f));
    spawnFollowers(KurorushiR2,  glm::vec3(-52.370f, 1.5f, -157.365f));
    spawnFollowers(KurorushiC1,  glm::vec3(150.490f, 1.5f, -169.563f));
    spawnFollowers(KurorushiC2,  glm::vec3(135.006f, 1.5f, -140.760f));

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

    bool isCrematoriumGearSpawned = false;

    // Main loop
    /*decorSystem.LoadInstancesFromYaml(
    "res/level1_decorations.yaml",
    availablePrefabs,
    *scena1,
    nullptr);*/

    sceneManager.Update(0.16f);
    sceneManager.ChangeScene("menu");
    sceneManager.UpdateChangeScene();

    CameraHelper::ProcessMouseMovement(*camCompLeft, *camera1->GetComponent<TransformComponent>(), 0.0f, 0.05f);
    CameraHelper::ProcessMouseMovement(*camCompRight, *camera2->GetComponent<TransformComponent>(), 0.0f, 0.05f);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        updateFPS(deltaTime);

        CpuTimer cpuTimer;
        cpuTimer.start();

        sceneManager.UpdateChangeScene();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // ostroznie przy zmianie rozdzielczosci recznie ciagnac myszka.
        // zmiana rozdzielczosci powinna tylko byc mozliwa poprzez ustawienia gry jak juz beda istniec
        if (display_h > 0) {
            float scaleY = (float)display_h / 1080.0f;

            if (p1NoteUI) {
                p1NoteUI->size = glm::vec2(500.0f * scaleY, 700.0f * scaleY);
                p1NoteUI->textOffset = glm::vec2(30.0f * scaleY, 30.0f * scaleY);

                p1NoteUI->fontPath = "res/fonts/Roboto-Regular.ttf";
                p1NoteUI->fontSize = 15.0f * scaleY;

                p1NoteUI->fontPath2 = "res/fonts/NothingYouCouldDo-Regular.ttf";
                p1NoteUI->fontSize2 = 18.0f * scaleY;

                p1NoteUI->screenPosition = glm::vec2((display_w * 0.25f) - (p1NoteUI->size.x * 0.5f),
                    (display_h * 0.5f) - (p1NoteUI->size.y * 0.5f));
            }

            if (p2NoteUI) {
                p2NoteUI->size = glm::vec2(500.0f * scaleY, 700.0f * scaleY);
                p2NoteUI->textOffset = glm::vec2(30.0f * scaleY, 30.0f * scaleY);

				p2NoteUI->fontPath = "res/fonts/Roboto-Regular.ttf";
				p2NoteUI->fontSize = 15.0f * scaleY;

				p2NoteUI->fontPath2 = "res/fonts/NothingYouCouldDo-Regular.ttf";
                p2NoteUI->fontSize2 = 18.0f * scaleY;

                p2NoteUI->screenPosition = glm::vec2((display_w * 0.75f) - (p2NoteUI->size.x * 0.5f),
                    (display_h * 0.5f) - (p2NoteUI->size.y * 0.5f));
            }
        }

        //Zerowanie predkosc graczy
        rigidBodyCamera1->velocity.x = 0.0f;
        rigidBodyCamera1->velocity.z = 0.0f;
        rigidBodyCamera1->acceleration.x = 0.0f;
        rigidBodyCamera1->acceleration.z = 0.0f;
        rigidBodyCamera2->velocity.x = 0.0f;
        rigidBodyCamera2->velocity.z = 0.0f;
        rigidBodyCamera2->acceleration.x = 0.0f;
        rigidBodyCamera2->acceleration.z = 0.0f;

        bool allFilled = IsPuzzleSolved();

        for (auto& [slotGO, slot] : puzzleSlotsMap) {
            if (slot.lightObject == nullptr) continue;
            LightComponent* light = slot.lightObject->GetComponent<LightComponent>();
            if (light == nullptr) continue;

            if (!allFilled) {
                light->isOn = false;
                light->diffuse = glm::vec3(0.0f);
                light->ambient = glm::vec3(0.0f);
                light->specular = glm::vec3(0.0f);
            }
            else if (slot.occupant == slot.expectedObject) {
                light->isOn = true;
                light->diffuse = glm::vec3(0.0f, 1.0f, 0.0f);
                light->ambient = glm::vec3(0.0f, 0.1f, 0.0f);
                light->specular = glm::vec3(0.0f, 0.5f, 0.0f);
            }
            else {
                light->isOn = true;
                light->diffuse = glm::vec3(1.0f, 0.0f, 0.0f);
                light->ambient = glm::vec3(0.1f, 0.0f, 0.0f);
                light->specular = glm::vec3(0.5f, 0.0f, 0.0f);
            }
        }

        UpdateDoors(deltaTime, audioSys, sndDoorClosed);
        UpdateCabinets(deltaTime);
   


        auto inputStart = std::chrono::high_resolution_clock::now();

        if (ecs->GetSystem<HID>()->is_action_just_pressed("right_click")) {
            focused = !focused;
            updateFocus();
        }
        if (ecs->GetSystem<HID>()->is_action_just_pressed("toggle_frustum_culling")) {
            renderSystem->frustumCullingEnabled = !renderSystem->frustumCullingEnabled;
            spdlog::info("Frustum culling: {}",
                renderSystem->frustumCullingEnabled ? "ON" : "OFF");
        }
        if (ecs->GetSystem<HID>()->is_action_just_pressed("toggle_oclussion_culling")) {
            renderSystem->occlusionCullingEnabled = !renderSystem->occlusionCullingEnabled;
            spdlog::info("Oclussion culling: {}",
                renderSystem->frustumCullingEnabled ? "ON" : "OFF");
        }

        if (ecs->GetSystem<HID>()->is_action_just_pressed("ui_menu")) {
            if (sceneManager.GetActiveSceneName() == "Scena 1")
            {
                sceneManager.ChangeScene("menu");
            }

            if (sceneManager.GetActiveSceneName() == "menu")
            {
                sceneManager.ChangeScene("Scena 1");
            }
        }

        if (ecs->GetSystem<HID>()->is_action_just_pressed("gamma_up")) {
            postProcessingSystem->set_gamma(postProcessingSystem->get_gamma() + 0.1f);
        }
        if (ecs->GetSystem<HID>()->is_action_just_pressed("gamma_down")) {
            postProcessingSystem->set_gamma(postProcessingSystem->get_gamma() - 0.1f);
        }

        if (ecs->GetSystem<HID>()->is_action_just_pressed("play_sound")) {
            ecs->GetSystem<AudioSystem>()->playSound(sound);
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
                    else
                        hintText = "Requires puzzle piece";
                }
                else if (machineSlotsMap.count(hit.hitObject)) {
                    PuzzleSlot& slot = machineSlotsMap[hit.hitObject];
                    if (slot.occupant != nullptr) {
                        hintText = "Gear Inserted";
                    }
                    else if (p1HeldObject != nullptr && p1HeldObject->name.find("Gear") != std::string::npos) {
                        hintText = "Insert Gear";
                    }
                    else {
                        hintText = "Requires Gear";
                    }
                }
                else if (hit.hitObject == machineStartButton) {
                    bool allInserted = true;
                    for (auto& [mSlotGO, mSlot] : machineSlotsMap) {
                        if (mSlot.occupant == nullptr) { allInserted = false; break; }
                    }

                    if (isMachineFixed) hintText = "Machine Running";
                    else if (allInserted) hintText = "Start Machine";
                    else hintText = "Missing Gears";
                }
                else if (p1HeldObject != nullptr) {
                    hintText = "Drop";
                }
                else if (rotatableObjects.count(hit.hitObject))
                    hintText = "Rotate";
                else if (toiletDoorsMap.count(hit.hitObject))
                    hintText = toiletDoorsMap[hit.hitObject].isOpen ? "Close" : "Open";
                else if (cabinetsMap.count(hit.hitObject))
                    hintText = isCabinetButtonPushed ? "" : "Open Cabinet";
                else if (majorDoors.count(hit.hitObject))
                    hintText = can_open_door_1 ? "Open" : "Unlock";
                else if (pickupObjects.count(hit.hitObject)) {
                    bool isInSlot = false;
                    for (auto& [slotGO, slot] : puzzleSlotsMap)
                        if (slot.occupant == hit.hitObject) { isInSlot = true; break; }
                    hintText = isInSlot ? "Pull out" : (hit.hitObject == p2HeldObject ? "" : "Pick up");
                }
                else if (hit.hitObject->name.find("Coffin") != std::string::npos)
                    hintText = "Pull Coffin";
                else if (noteContents.count(hit.hitObject)) {
                    hintText = "Read Note";
                }
            }
        }
        else if (p1HeldObject != nullptr) {
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
                else if (machineSlotsMap.count(hit.hitObject)) {
                    PuzzleSlot& slot = machineSlotsMap[hit.hitObject];
                    if (slot.occupant != nullptr) {
                        hintText2 = "Gear Inserted";
                    }
                    else if (p2HeldObject != nullptr && p2HeldObject->name.find("Gear") != std::string::npos) {
                        hintText2 = "Insert Gear";
                    }
                    else {
                        hintText2 = "Requires Gear";
                    }
                }
                else if (hit.hitObject == machineStartButton) {
                    bool allInserted = true;
                    for (auto& [mSlotGO, mSlot] : machineSlotsMap) {
                        if (mSlot.occupant == nullptr) { allInserted = false; break; }
                    }

                    if (isMachineFixed) hintText2 = "Machine Running";
                    else if (allInserted) hintText2 = "Start Machine";
                    else hintText2 = "Missing Gears";
                }
                else if (p2HeldObject != nullptr) {
                    hintText2 = "Drop";
                }
                else if (rotatableObjects.count(hit.hitObject))
                    hintText2 = "Rotate";
                else if (toiletDoorsMap.count(hit.hitObject))
                    hintText2 = toiletDoorsMap[hit.hitObject].isOpen ? "Close" : "Open";
                else if (cabinetsMap.count(hit.hitObject))
                    hintText2 = isCabinetButtonPushed ? "" : "Open Cabinet";
                else if (majorDoors.count(hit.hitObject))
                    hintText2 = can_open_door_1 ? "Open" : "Unlock";
                else if (pickupObjects.count(hit.hitObject)) {
                    bool isInSlot = false;
                    for (auto& [slotGO, slot] : puzzleSlotsMap)
                        if (slot.occupant == hit.hitObject) { isInSlot = true; break; }
                    hintText2 = isInSlot ? "Pull out" : (hit.hitObject == p1HeldObject ? "" : "Pick up");
                }
                else if (hit.hitObject->name.find("Coffin") != std::string::npos)
                    hintText2 = "Pull Coffin";
                else if (noteContents.count(hit.hitObject)) {
                    hintText2 = "Read Note";
                }
            }
        }
        else if (p2HeldObject != nullptr) {
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
                spdlog::info("Rotated to: {:.2f}", transform->rotation.z);
                transform->isDirty = false;
                rotatingInProgress.erase(it->first);
                it = rotatingObjects.erase(it);
                checkKibelUstawienia();
            }
            else ++it;
        }

        if (focused) {
            static bool rentgenPuzzleSolvedPlayed = false;

            if (ecs->GetSystem<HID>()->is_action_just_pressed("reset_level")) {
                rentgenPuzzleSolvedPlayed = false;

                ResetLevel(
                    scena1,
                    gracz1, gracz2,
                    t0, t1,
                    rigidBodyCamera1, rigidBodyCamera2,
                    p1HeldObject, p2HeldObject,
                    p1IsReading, p2IsReading,
                    p1NoteUI, p2NoteUI,
                    isCrematoriumGearSpawned,
                    rentgenPuzzleSolvedPlayed,
                    roomsLights,
                    [](int id) { InitializeRoomLights(id); }
                );
            }
            if (ecs->GetSystem<HID>()->is_action_just_pressed("interact_p1")) {
                if (p1IsReading) {
                    p1IsReading = false;
                    p1NoteUI->isVisible = false;
                    if (audioSys && sndLorePaper) audioSys->playSound(sndLorePaper);
                }
                else if (player1Raycast->anyHit()) {
                    auto hit = player1Raycast->closestHit();
                    if (hit.hitObject && noteContents.count(hit.hitObject)) {
                        p1IsReading = true;
                        p1NoteUI->text = noteContents[hit.hitObject];
                        p1NoteUI->isVisible = true;
                        if (audioSys && sndLorePaper) audioSys->playSound(sndLorePaper);
                    }
                }
            }

            if (!p1IsReading) {
                HandlePlayerInteraction(*ecs, "interact_p1", player1Raycast, camera1, p1HeldObject, p2HeldObject, scena1, rotatingObjects, rotatingInProgress, p1ShakeTimer, p1Animator, postacGraczaCzerw.get(), audioSys, sndPaperRoll, sndDoorOpen, sndDoorCloseStart, sndBtnClick, sound, sndPickup, sndInsert, sndDoorLocked, sndGear);
                HandleAltRotate(*ecs, "alt_interact_p1", player1Raycast, rotatingObjects, rotatingInProgress, audioSys, sndPaperRoll);
            }

            if (ecs->GetSystem<HID>()->is_action_just_pressed("interact_p2")) {
                if (p2IsReading) {
                    p2IsReading = false;
                    p2NoteUI->isVisible = false;
                    if (audioSys && sndLorePaper) audioSys->playSound(sndLorePaper);
                }
                else if (player2Raycast->anyHit()) {
                    auto hit = player2Raycast->closestHit();
                    if (hit.hitObject && noteContents.count(hit.hitObject)) {
                        p2IsReading = true;
                        p2NoteUI->text = noteContents[hit.hitObject];
                        p2NoteUI->isVisible = true;
                        if (audioSys && sndLorePaper) audioSys->playSound(sndLorePaper);
                    }
                }
            }

            if (!p2IsReading) {
                HandlePlayerInteraction(*ecs, "interact_p2", player2Raycast, camera2, p2HeldObject, p1HeldObject, scena1, rotatingObjects, rotatingInProgress, p2ShakeTimer, p2Animator, postacGraczaZiel.get(), audioSys, sndPaperRoll, sndDoorOpen, sndDoorCloseStart, sndBtnClick, sound, sndPickup, sndInsert, sndDoorLocked, sndGear);
                HandleAltRotate(*ecs, "alt_interact_p2", player2Raycast, rotatingObjects, rotatingInProgress, audioSys, sndPaperRoll);
            }
        }

        auto updateHeldGear = [&](GameObject* heldObj) {
            if (heldObj && heldObj->name.find("Gear") != std::string::npos) {
                if (auto tr = heldObj->GetComponent<TransformComponent>()) {
                    tr->position = gearHeldOffset;
                    tr->rotation = gearHeldRotation;
                    tr->isDirty = true;
                }
            }
            };
        updateHeldGear(p1HeldObject);
        updateHeldGear(p2HeldObject);

        //// testy animacji
        //if (ecs->GetSystem<HID>()->is_action_just_pressed("anim_play_dying")) {
        //    auto* clip = AnimationHelper::FindAnimation(dyingModelPrefab->rootModel->animations, "mixamo.com");
        //    if (clip) {
        //        AnimationHelper::Play(animator, clip, true, 1.0f);
        //        spdlog::info("Odtworzono animacje umierania");
        //    }
        //}

        //if (ecs->GetSystem<HID>()->is_action_just_pressed("anim_play_jump")) {
        //    auto* clip = &jumpSkeletonPrefab->rootModel->animations[0];
        //    if (clip) {
        //        AnimationHelper::Play(animator, clip, true, 1.0f);
        //        spdlog::info("Odtworzono animacje skoku");
        //    }
        //}

        //if (ecs->GetSystem<HID>()->is_action_pressed("anim_slow_mo")) {
        //    animator->playbackSpeed = 0.5f;
        //}
        //else if (ecs->GetSystem<HID>()->is_action_pressed("anim_fast_forward")) {
        //    animator->playbackSpeed = 2.0f;
        //}
        //else {
        //    animator->playbackSpeed = 1.0f;
        //}

        input();
        CheckFallenPickupObjects();

        bool p1IsMoving = false;
        bool p2IsMoving = false;
        bool p1IsTurning = false;
        bool p2IsTurning = false;

        if (focused) {
            if (!p1IsReading) {
                p1IsMoving |= processCameraInput(*ecs, *camCompLeft, *t0, "move_up", "move_down", "move_left", "move_right");
                p1IsTurning |= processCameraMouse(*ecs, *camCompLeft, *camTransform1, *t0);
                bool g1Turning = false;
                p1IsMoving |= processCameraGamepad(*ecs, *camCompLeft, *camTransform1, *t0, 0, g1Turning);
                p1IsTurning |= g1Turning;
            }

            if (!p2IsReading) {
                p2IsMoving |= processCameraInput(*ecs, *camCompRight, *t1, "move_up_2", "move_down_2", "move_left_2", "move_right_2");
                bool g2Turning = false;
                p2IsMoving |= processCameraGamepad(*ecs, *camCompRight, *camTransform2, *t1, 1, g2Turning);
                p2IsTurning |= g2Turning;
            }
        }

        bool p1IsHolding = (p1HeldObject != nullptr);
        bool p2IsHolding = (p2HeldObject != nullptr);

        PlayerAnimationHelper::UpdateAnimation(p1Animator, postacGraczaCzerw.get(), p1IsMoving, p1IsHolding, p1IsTurning);
        PlayerAnimationHelper::UpdateAnimation(p2Animator, postacGraczaZiel.get(), p2IsMoving, p2IsHolding, p2IsTurning);

        if (p1ShakeTimer > 0.0f) p1ShakeTimer -= deltaTime;
        if (p2ShakeTimer > 0.0f) p2ShakeTimer -= deltaTime;

        if (p1ShakeTimer > 0.0f) {
            float t = currentFrame * 40.0f;
            float strength = 6.0f;
            player1InteractionInfo->screenPosition = p1BasePos + glm::vec2(
                sin(t) * strength,
                sin(t * 1.3f) * strength * 0.5f
            );
        }
        else {
            player1InteractionInfo->screenPosition = p1BasePos;
        }

        if (p2ShakeTimer > 0.0f) {
            float t = currentFrame * 40.0f;
            float strength = 6.0f;
            player2InteractionInfo->screenPosition = p2BasePos + glm::vec2(
                sin(t + 1.0f) * strength,
                sin(t * 1.3f + 1.0f) * strength * 0.5f
            );
        }
        else {
            player2InteractionInfo->screenPosition = p2BasePos;
        }

        float p1Int = 0.0f, p2Int = 0.0f;

        if (player1Raycast->anyHit()) {
            auto hit = player1Raycast->closestHit();
            if (hit.hitObject &&
                (rotatableObjects.count(hit.hitObject) ||
                    toiletDoorsMap.count(hit.hitObject) ||
                    cabinetsMap.count(hit.hitObject) ||
                    majorDoors.count(hit.hitObject) ||
                    pickupObjects.count(hit.hitObject) ||
                    hit.hitObject->name.find("Coffin") != std::string::npos ||
                    (puzzleSlotsMap.count(hit.hitObject) && puzzleSlotsMap[hit.hitObject].occupant != nullptr) ||
                    machineSlotsMap.count(hit.hitObject) ||
                    hit.hitObject == machineStartButton))
                p1Int = 1.0f;
        }
        if (player2Raycast->anyHit()) {
            auto hit = player2Raycast->closestHit();
            if (hit.hitObject &&
                (rotatableObjects.count(hit.hitObject) ||
                    toiletDoorsMap.count(hit.hitObject) ||
                    cabinetsMap.count(hit.hitObject) ||
                    majorDoors.count(hit.hitObject) ||
                    pickupObjects.count(hit.hitObject) ||
                    hit.hitObject->name.find("Coffin") != std::string::npos ||
                    (puzzleSlotsMap.count(hit.hitObject) && puzzleSlotsMap[hit.hitObject].occupant != nullptr) ||
                    machineSlotsMap.count(hit.hitObject) ||
                    hit.hitObject == machineStartButton))
                p1Int = 1.0f;
        }

        float chLerpSpeed = 10.0f;
        crosshair1->size = glm::mix(crosshair1->size, p1Int > 0.5f ? CH_SIZE_BIG : CH_SIZE_NORMAL, deltaTime * chLerpSpeed);
        crosshair2->size = glm::mix(crosshair2->size, p2Int > 0.5f ? CH_SIZE_BIG : CH_SIZE_NORMAL, deltaTime * chLerpSpeed);

        crosshair1->screenPosition = CH1_CENTER - crosshair1->size * 0.5f;
        crosshair2->screenPosition = CH2_CENTER - crosshair2->size * 0.5f;

        auto inputEnd = std::chrono::high_resolution_clock::now();

        auto logicStart = std::chrono::high_resolution_clock::now();
        crematoriumPuzzle.Update(deltaTime);

        if (crematoriumPuzzle.isPuzzleSolved && !isCrematoriumGearSpawned) {
            SpawnGearReward(scena1, glm::vec3(145.984f, 7.309, -129.553), "Gear_Crematorium");

            if (audioSys && sndGear) {
                audioSys->playSound(sndGear);
            }

            isCrematoriumGearSpawned = true;
        }

        if (isMachineFixed) {
            float baseSpeed = 100.0f * deltaTime;

            for (auto& [mSlotGO, mSlot] : machineSlotsMap) {
                if (mSlot.occupant != nullptr) {
                    if (auto tr = mSlot.occupant->GetComponent<TransformComponent>()) {

                        float dir = 1.0f;
                        if (mSlotGO->name == "MachineSlot_3") dir = -1.0f;

                        tr->rotation.x += baseSpeed * dir;
                        tr->isDirty = true;
                    }
                }
            }

            if (fixedGear1) {
                if (auto tr = fixedGear1->GetComponent<TransformComponent>()) {
                    tr->rotation.x += baseSpeed * 1.0f;
                    tr->isDirty = true;
                }
            }

            if (fixedGear2) {
                if (auto tr = fixedGear2->GetComponent<TransformComponent>()) {
                    tr->rotation.x += baseSpeed * -1.0f;
                    tr->isDirty = true;
                }
            }
        }

        TransformGizmo::UpdateAndDraw(
    selectedGameObject,
    *camCompLeft, *camTransform1,
    window, focused,
    display_w, display_h
);
        sceneManager.Update(deltaTime);

        if (audioSys) {
            audioSys->Update(*ecs, deltaTime);
        }

        update();
        auto logicEnd = std::chrono::high_resolution_clock::now();

        imgui_begin();
        imgui_render(sceneManager);
        imgui_end();


        cpuTimer.stop();

        float cpuFrameTime = cpuTimer.getMilliseconds();
        float logicTime = std::chrono::duration<float, std::milli>(logicEnd - logicStart).count();
        float inputTime = std::chrono::duration<float, std::milli>(inputEnd - inputStart).count();

        perf.cpuFrameTime = cpuFrameTime;
        perf.logicTime = logicTime;
        perf.inputTime = inputTime;

        end_frame();
    }

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
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        spdlog::error("Failed to initalize GLFW!");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MimiCry", NULL, NULL);
    if (window == NULL) {
        spdlog::error("Failed to create GLFW Window!");
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window,       mouse_callback);
    glfwSetScrollCallback(window,          scroll_callback);

    bool err = !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    if (err) {
        spdlog::error("Failed to initialize OpenGL loader!");
        return false;
    }
    return true;
}

void init_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsDark();
}

void compileShader()
{
    spdlog::info("Success");
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

#include "impl/main/imgui.ipp"

void end_frame()
{
    glfwPollEvents();
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}


//void addAllSystems(ECS& ecs) {
//    ecs.AddSystem<TransformSystem>(ecs);
//    ecs.AddSystem<PhysicsSystem>(ecs);
//    ecs.AddSystem<AnimationSystem>(ecs);
//    ecs.AddSystem<RenderSystem>(ecs, window);
//    ecs.AddSystem<HID>(ecs, window);
//    ecs.AddSystem<PostProcessingSystem>(ecs, window);
//    ecs.AddSystem<SpriteSystem>(ecs, window);
//    ecs.AddSystem<RaycastSystem>(ecs);
//    ecs.AddSystem<NavMeshSystem>(ecs);
//    ecs.AddSystem<NavPathSystem>(ecs);
//    ecs.AddSystem<AudioSystem>(ecs);
//    ecs.AddSystem<NpcSystem>(ecs);
//}

void LoadPlayerAnimations(Prefab* postacGracza) {

    if (!postacGracza || !postacGracza->rootModel) return;

    std::vector<AnimationClip> rawAnimations = postacGracza->rootModel->animations;
    postacGracza->rootModel->animations.clear();
    postacGracza->rootModel->animations.resize(9);

    auto findAndMap = [&](const std::string& exactName, int targetIdx, const std::string& fallbackName = "") {
        for (auto& anim : rawAnimations) {
            if (anim.name == exactName) {
                postacGracza->rootModel->animations[targetIdx] = anim;
                return true;
            }
        }

        for (auto& anim : rawAnimations) {
            if (anim.name.find(exactName) != std::string::npos) {
                postacGracza->rootModel->animations[targetIdx] = anim;
                return true;
            }
        }

        if (!fallbackName.empty()) {
            for (auto& anim : rawAnimations) {
                if (anim.name.find(fallbackName) != std::string::npos) {
                    postacGracza->rootModel->animations[targetIdx] = anim;
                    return true;
                }
            }
        }
        return false;
        };

    findAndMap("idle", PlayerAnimationHelper::IDLE_ANIM_INDEX);
    findAndMap("idle_pick", PlayerAnimationHelper::IDLE_HOLD_INDEX);
    findAndMap("interaction", PlayerAnimationHelper::INTERACT_ANIM_INDEX);
    findAndMap("pick", PlayerAnimationHelper::PICKUP_ANIM_INDEX);
    findAndMap("throw_away", PlayerAnimationHelper::DROP_ANIM_INDEX);
    findAndMap("turn_around", PlayerAnimationHelper::TURN_AROUND_ANIM_INDEX);
    findAndMap("turn_around_pick", PlayerAnimationHelper::TURN_AROUND_HOLD_INDEX);
    findAndMap("walk", PlayerAnimationHelper::WALK_ANIM_INDEX);
    findAndMap("walk_pick", PlayerAnimationHelper::WALK_HOLD_INDEX);
}

#include "impl/main/room_creator.ipp"
