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
#include "systems/SurfaceDecorationSystem.h"


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

bool processCameraGamepad(ECS& ecs, CameraComponent& cam, TransformComponent& transformCamera, TransformComponent& playerTransform, int gamepad_id);
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

bool sceneIsMenu = false;
bool prevSceneIsMenu = false;


unsigned int cubemapTexture;
unsigned int skyboxVAO;

CrematoriumPuzzle crematoriumPuzzle;
SurfaceDecorationSystem decorSystem;

GLuint VBO;
GLuint VAO;
GLuint texture;
std::unique_ptr<Shader> skyboxShader;
//std::unique_ptr<Shader> reflectShader;
//std::unique_ptr<Shader> refractShader;

std::unique_ptr<Prefab> postacGracza;
std::unique_ptr<Prefab> bed1Model;
std::unique_ptr<Prefab> bed2Model;
std::unique_ptr<Prefab> bed3Model;
std::unique_ptr<Prefab> doorsModel;
std::unique_ptr<Prefab> krzesloModel;
std::unique_ptr<Prefab> lampa1Model;
std::unique_ptr<Prefab> lampa2Model;
std::unique_ptr<Prefab> lampa3Model;
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
std::unique_ptr<Prefab> szafa1Model;
std::unique_ptr<Prefab> szafa2Model;
std::unique_ptr<Prefab> szafa3Model;
std::unique_ptr<Prefab> telephoneModel;
std::unique_ptr<Prefab> toiletModel;
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
std::unique_ptr<Prefab> szafka_labModel;

std::unique_ptr<Prefab> puzel1;
std::unique_ptr<Prefab> puzel2;
std::unique_ptr<Prefab> puzel3;
std::unique_ptr<Prefab> puzel4;
std::unique_ptr<Prefab> puzel5;
std::unique_ptr<Prefab> puzel6;

std::unique_ptr<Prefab> czerwonaTablica;
std::unique_ptr<Prefab> zielonaTablica;
std::unique_ptr<Prefab> Rentgen;
std::unique_ptr<Prefab> SzafkaRentgen2Model;
std::unique_ptr<Prefab> SzafkaRentgen1Model;
std::unique_ptr<Prefab> lampaOperacyjnaModel;
std::unique_ptr<Prefab> stolOperacyjnyModel;
std::unique_ptr<Prefab> zaslonaModel;

std::unique_ptr<Prefab> kredensModel;
std::unique_ptr<Prefab> eksp1Model;
std::unique_ptr<Prefab> fiolka2Model;
std::unique_ptr<Prefab> fiolka1Model;
std::unique_ptr<Prefab> ksiazkaModel;
std::unique_ptr<Prefab> eksp2Model;
std::unique_ptr<Prefab> eksp3Model;
std::unique_ptr<Prefab> eksp4Model;
std::unique_ptr<Prefab> wozekModel;
std::unique_ptr<Prefab> szafka_inna1Model;
std::unique_ptr<Prefab> szafka_inna2Model;
std::unique_ptr<Prefab> fiolka_nastModel;
std::unique_ptr<Prefab> deskModel;
std::unique_ptr<Prefab> drawer1Model;
std::unique_ptr<Prefab> drawer2Model;
std::unique_ptr<Prefab> needleModel;
std::unique_ptr<Prefab> sinkModel;
std::unique_ptr<Prefab> tableModel;

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
std::unique_ptr<Prefab> pokrywkaRolkiModel;

std::unique_ptr<Prefab> dyingModelPrefab;
std::unique_ptr<Prefab> jumpSkeletonPrefab;

// testowe obiekty do postprocessingu
std::unique_ptr<Prefab> RedModel;
std::unique_ptr<Prefab> BlueModel;
std::unique_ptr<Prefab> GreenModel;

std::unique_ptr<Prefab> roofModel;
std::unique_ptr<Prefab> groundModel;

std::unique_ptr<Prefab> koparkaModel;

#include "impl/main/list_prefab.ipp";

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

void LoadPlayerAnimations();
void createRentgenCorridor(Scene * scena);
void createCrematoriumCorridor(Scene * scena);

#include "impl/main/logic_update.ipp"

void createFirstRoom(Scene* scena1);
void createMainRooom(Scene* scena);
void createNuclearRooom(Scene* scena);
void createCrematorium(Scene* scena);
void createRentgenRoom(Scene* scena);

void ChangeScene(SceneManager* sceneManager, ECS*& ecs)
{

    if (sceneIsMenu)
    {
        sceneManager->SetActiveScene("menu", window);
        Scene* active = sceneManager->GetActiveScene();
        ecs = &active->GetECS();
        renderSystem = ecs->GetSystem<RenderSystem>();
        postProcessingSystem = ecs->GetSystem<PostProcessingSystem>();
    }
    else
    {
        sceneManager->SetActiveScene("Scena 1", window);
        Scene* active = sceneManager->GetActiveScene();
        ecs = &active->GetECS();
        renderSystem = ecs->GetSystem<RenderSystem>();
        postProcessingSystem = ecs->GetSystem<PostProcessingSystem>();
    }

    prevSceneIsMenu = sceneIsMenu;
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

    SceneManager sceneManager;

    sceneManager.CreateScene("Scena 1");
    sceneManager.CreateScene("menu");

    Scene* scena1 = sceneManager.GetActiveScene();
    Scene* menu = sceneManager.GetScene("menu");
    ECS* ecs = &scena1->GetECS();

    scena1->addAllSystems(window);
    menu->GetECS().AddSystem<HID>(menu->GetECS(), window);
    menu->GetECS().AddSystem<TransformSystem>(menu->GetECS());
    menu->GetECS().AddSystem<RenderSystem>(menu->GetECS(), window);
    menu->GetECS().AddSystem<SpriteSystem>(menu->GetECS(), window);
    menu->GetECS().AddSystem<PostProcessingSystem>(menu->GetECS(), window);
    menu->GetECS().GetSystem<PostProcessingSystem>()->SetActive(false);
    groundModel = std::make_unique<Prefab>("res/models/podloze.glb");

    GameObject* cameraMenu = menu->CreateGameObject(nullptr);//groundModel->Instantiate(*scena1, nullptr, ourShader.get());
    cameraMenu->name = "Kamera";
    cameraMenu->AddComponent<CameraComponent>();
    GameObject* menuPodloze = groundModel->Instantiate(*menu, nullptr, nullptr);

    sceneManager.SetActiveScene("Scena 1", window);
    //menu->GetECS().AddExistingSystem(scena1->GetECS().GetSystem<RenderSystem>());

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
    gracz1->GetComponent<TransformComponent>()->position = glm::vec3(11.986f, 6.250f, -12.000f);
    gracz1->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
    gracz1->GetComponent<RigidbodyComponent>()->mass = 10.0f;
    gracz1->GetComponent<RigidbodyComponent>()->bounce = 0.1f;
    gracz1->GetComponent<RigidbodyComponent>()->useGravity = true;
    gracz1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.0f, 5.25f, 1.0f };

    
    GameObject* camera1 = scena1->CreateGameObject(nullptr);//groundModel->Instantiate(*scena1, nullptr, ourShader.get());
    camera1->name = "Kamera";
    gracz1->AddChild(camera1);
    camera1->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 4.7f, 0.0f);
    CameraComponent* camCompLeft = camera1->AddComponent<CameraComponent>();
    RaycastComponent*  player1Raycast   = camera1->AddComponent<RaycastComponent>();
    player1Raycast->debugDraw = false;

    GameObject* latarka1 = scena1->CreateGameObject(nullptr);
    latarka1->name = "Latarka";
    camera1->AddChild(latarka1);
    latarka1->GetComponent<TransformComponent>()->position = glm::vec3(1.5f, -1.0f, 0.5f);
    latarka1->GetComponent<TransformComponent>()->rotation = glm::vec3(3.0f, 4.5f, 0.0f);
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

    GameObject* modelPostac1 = postacGracza->Instantiate(*scena1, nullptr, nullptr);
    gracz1->AddChild(modelPostac1);
    modelPostac1->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 0.9f, 1.7f);
    modelPostac1->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
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
    gracz2->GetComponent<TransformComponent>()->position = glm::vec3(0.070, 6.250f, -18.649f);
    gracz2->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
    gracz2->GetComponent<RigidbodyComponent>()->mass = 10.0f;
    gracz2->GetComponent<RigidbodyComponent>()->bounce = 0.1f;
    gracz2->GetComponent<RigidbodyComponent>()->useGravity = true;
    gracz2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1.0f, 5.25f, 1.0f };

    GameObject* camera2 = scena1->CreateGameObject(nullptr);
    camera2->name = "Kamera";
    gracz2->AddChild(camera2);
    camera2->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 4.7f, 0.0f);
    CameraComponent* camCompRight = camera2->AddComponent<CameraComponent>();
    RaycastComponent* player2Raycast = camera2->AddComponent<RaycastComponent>();
    player2Raycast->debugDraw = false;

    GameObject* latarka2 = scena1->CreateGameObject(nullptr);
    latarka2->name = "Latarka";
    camera2->AddChild(latarka2);
    latarka2->GetComponent<TransformComponent>()->position = glm::vec3(1.5f, -1.0f, 0.5f);
    latarka2->GetComponent<TransformComponent>()->rotation = glm::vec3(3.0f, 4.5f, 0.0f);
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

    GameObject* modelPostac2 = postacGracza->Instantiate(*scena1, nullptr, nullptr);
    gracz2->AddChild(modelPostac2);
    modelPostac2->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 0.9f, 1.7f);
    modelPostac2->GetComponent<TransformComponent>()->rotation = glm::vec3(0.0f, -180.0f, 0.0f);
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


    renderSystem         = ecs->GetSystem<RenderSystem>();
    postProcessingSystem = ecs->GetSystem<PostProcessingSystem>();

    createFirstRoom(scena1);
    createMainRooom(scena1);
    createNuclearRooom(scena1);
    createCrematorium(scena1);
    createRentgenRoom(scena1);
    createRentgenCorridor(scena1);
    createCrematoriumCorridor(scena1);

    ecs->GetSystem<NavMeshSystem>()->Bake(*scena1);

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
    audioSys->createSound("res/sound/button_click.wav", sndBtnClick, false);
    audioSys->createSound("res/sound/pick_up.wav", sndPickup, false);
    audioSys->createSound("res/sound/insert_puzzle.wav", sndInsert, false);
    audioSys->createSound("res/sound/door_locked.wav", sndDoorLocked, false);
    audioSys->createSound("res/sound/falling_gear.wav", sndGear, false);

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

    bool isCrematoriumGearSpawned = false;

    // Main loop
    /*decorSystem.LoadInstancesFromYaml(
    "res/level1_decorations.yaml",
    availablePrefabs,
    *scena1,
    nullptr);*/
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        updateFPS(deltaTime);

        CpuTimer cpuTimer;
        cpuTimer.start();


        if (prevSceneIsMenu != sceneIsMenu)
        {
            ChangeScene(&sceneManager, ecs);
        }

        bool allFilled = !puzzleSlotsMap.empty() && std::all_of(
            puzzleSlotsMap.begin(), puzzleSlotsMap.end(),
            [](const auto& pair) { return pair.second.occupant != nullptr; }
        );

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

        if (ecs->GetSystem<HID>()->is_action_just_pressed("gamma_up")) {
            postProcessingSystem->set_gamma(postProcessingSystem->get_gamma() + 0.1f);
            sceneIsMenu = !sceneIsMenu;
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
            HandlePlayerInteraction(*ecs, "interact_p1", player1Raycast, camera1, p1HeldObject, p2HeldObject, scena1, rotatingObjects, rotatingInProgress, p1ShakeTimer, p1Animator, postacGracza.get(), audioSys, sndPaperRoll, sndDoorOpen, sndDoorCloseStart, sndBtnClick, sound, sndPickup, sndInsert, sndDoorLocked, sndGear);
            HandlePlayerInteraction(*ecs, "interact_p2", player2Raycast, camera2, p2HeldObject, p1HeldObject, scena1, rotatingObjects, rotatingInProgress, p2ShakeTimer, p2Animator, postacGracza.get(), audioSys, sndPaperRoll, sndDoorOpen, sndDoorCloseStart, sndBtnClick, sound, sndPickup, sndInsert, sndDoorLocked, sndGear);

            HandleAltRotate(*ecs, "alt_interact_p1", player1Raycast, rotatingObjects, rotatingInProgress, audioSys, sndPaperRoll);
            HandleAltRotate(*ecs, "alt_interact_p2", player2Raycast, rotatingObjects, rotatingInProgress, audioSys, sndPaperRoll);
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

        // testy animacji
        if (ecs->GetSystem<HID>()->is_action_just_pressed("anim_play_dying")) {
            auto* clip = AnimationHelper::FindAnimation(dyingModelPrefab->rootModel->animations, "mixamo.com");
            if (clip) {
                AnimationHelper::Play(animator, clip, true, 1.0f);
                spdlog::info("Odtworzono animacje umierania");
            }
        }

        if (ecs->GetSystem<HID>()->is_action_just_pressed("anim_play_jump")) {
            auto* clip = &jumpSkeletonPrefab->rootModel->animations[0];
            if (clip) {
                AnimationHelper::Play(animator, clip, true, 1.0f);
                spdlog::info("Odtworzono animacje skoku");
            }
        }

        if (ecs->GetSystem<HID>()->is_action_pressed("anim_slow_mo")) {
            animator->playbackSpeed = 0.5f;
        }
        else if (ecs->GetSystem<HID>()->is_action_pressed("anim_fast_forward")) {
            animator->playbackSpeed = 2.0f;
        }
        else {
            animator->playbackSpeed = 1.0f;
        }

        input();

        bool p1IsMoving = false;
        bool p2IsMoving = false;

        if (focused) {
            p1IsMoving |= processCameraInput(*ecs, *camCompLeft, *t0, "move_up", "move_down", "move_left", "move_right");
            p2IsMoving |= processCameraInput(*ecs, *camCompRight, *t1, "move_up_2", "move_down_2", "move_left_2", "move_right_2");

            processCameraMouse(*ecs, *camCompLeft, *camTransform1, *t0);

            p1IsMoving |= processCameraGamepad(*ecs, *camCompLeft, *camTransform1, *t0, 0);
            p2IsMoving |= processCameraGamepad(*ecs, *camCompRight, *camTransform2, *t1, 1);
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
            SpawnGearReward(scena1, glm::vec3(175.0f, 8.0f, -255.0f), "Gear_Crematorium");

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
                    tr->rotation.x += baseSpeed * -1.0f;
                    tr->isDirty = true;
                }
            }

            if (fixedGear2) {
                if (auto tr = fixedGear2->GetComponent<TransformComponent>()) {
                    tr->rotation.x += baseSpeed * 1.0f;
                    tr->isDirty = true;
                }
            }
        }

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

void LoadPlayerAnimations() {
    spdlog::info("Mapowanie animacji z pojedynczego pliku .glb gracza...");

    if (!postacGracza || !postacGracza->rootModel) return;

    std::vector<AnimationClip> rawAnimations = postacGracza->rootModel->animations;
    postacGracza->rootModel->animations.clear();
    postacGracza->rootModel->animations.resize(7);

    auto findAndMap = [&](const std::string& exactName, int targetIdx, const std::string& fallbackName = "") {
        for (auto& anim : rawAnimations) {
            if (anim.name == exactName) {
                postacGracza->rootModel->animations[targetIdx] = anim;
                spdlog::info("Zmapowano animację '{}' pod indeks [{}]", anim.name, targetIdx);
                return true;
            }
        }

        for (auto& anim : rawAnimations) {
            if (anim.name.find(exactName) != std::string::npos) {
                postacGracza->rootModel->animations[targetIdx] = anim;
                spdlog::info("Zmapowano (częściowo) animację '{}' pod indeks [{}]", anim.name, targetIdx);
                return true;
            }
        }

        if (!fallbackName.empty()) {
            for (auto& anim : rawAnimations) {
                if (anim.name.find(fallbackName) != std::string::npos) {
                    postacGracza->rootModel->animations[targetIdx] = anim;
                    spdlog::warn("Brak '{}', użyto zamiennika '{}' dla indeksu [{}]", exactName, anim.name, targetIdx);
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
    sinkModel        = std::make_unique<Prefab>("res/models/sink.glb");
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
    NormalDoor       = std::make_unique<Prefab>("res/models/doors_smaller.glb");
    szafkaModel      = std::make_unique<Prefab>("res/models/szafka_pop_main.glb");
    szafka_labModel      = std::make_unique<Prefab>("res/models/szafka_lab.glb");
    ruraModel        = std::make_unique<Prefab>("res/models/placeholder_rura_wysuwana.glb");
    panelModel       = std::make_unique<Prefab>("res/models/Panel.glb");
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
    pokrywkaRolkiModel = std::make_unique<Prefab>("res/models/do_rolki.glb");

    krzesloModel = std::make_unique<Prefab>("res/models/krzeslo.glb");
    wozekModel = std::make_unique<Prefab>("res/models/wozek.glb");
    eksp3Model = std::make_unique<Prefab>("res/models/eksperyment3.glb");
    eksp4Model = std::make_unique<Prefab>("res/models/eksperyment4.glb");
    szafka_inna1Model = std::make_unique<Prefab>("res/models/szafka_inna1.glb");
    szafka_inna2Model = std::make_unique<Prefab>("res/models/szafka_inna2.glb");
    fiolka_nastModel = std::make_unique<Prefab>("res/models/fiolka_nast.glb");
    telephoneModel = std::make_unique<Prefab>("res/models/telephone.glb");

    SzafkaRentgen2Model = std::make_unique<Prefab>("res/models/szafka2Rentgen.glb");
    SzafkaRentgen1Model = std::make_unique<Prefab>("res/models/szafka1Rentgen.glb");
    lampaOperacyjnaModel = std::make_unique<Prefab>("res/models/lampa_operac.glb");
    stolOperacyjnyModel = std::make_unique<Prefab>("res/models/stol_operacyjny.glb");
    zaslonaModel = std::make_unique<Prefab>("res/models/zaslona.glb");
    deskModel = std::make_unique<Prefab>("res/models/desk.glb");
    drawer1Model = std::make_unique<Prefab>("res/models/drawer1.glb");
    drawer2Model = std::make_unique<Prefab>("res/models/drawer2.glb");
    needleModel = std::make_unique<Prefab>("res/models/needle.glb");
    tableModel = std::make_unique<Prefab>("res/models/stol.glb");
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
    CreateStaticObject(scena1, floorModel.get(), nullptr, "SufitWKiblu",       glm::vec3(0, 20, 0),  glm::vec3(100, 1, 100), glm::vec3(0)); //glm::vec3(100, 1, 100)

    // Sciany
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "ScianaTylnaKibel",           glm::vec3(0, 0, -10),    glm::vec3(50, 50, 1),  glm::vec3(0)); // glm::vec3(50, 50, 1)
    CreateStaticObject(scena1, wallModel2.get(), nullptr, "ScianaKiblowa",              glm::vec3(35, 0, 0),     glm::vec3(100, 50, 1), glm::vec3(0,90,0)); // glm::vec3(1, 50, 100)
    CreateStaticObject(scena1, wallModel2.get(), nullptr, "ScianaSinkowa",              glm::vec3(-10, 0, 0),    glm::vec3(100, 50, 1), glm::vec3(0,90,0)); // glm::vec3(1, 50, 100)
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "ScianaDrzwiDoMainRoomPrawa", glm::vec3(110, 0, -100), glm::vec3(100, 50, 1), glm::vec3(0), glm::vec3(100,50,1), true);
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "ScianaDrzwiDoMainRoomLewa",  glm::vec3(-110, 0, -100),glm::vec3(110, 50, 1), glm::vec3(0), glm::vec3(110,50,1), true);
    CreateStaticObject(scena1, wallModel.get(),  nullptr, "GoraPrzejscieDoMainRoom",    glm::vec3(0, 70, -100),  glm::vec3(100, 50, 1), glm::vec3(0), glm::vec3(100,50,1), true);

    // Kibel
    GameObject* kible = scena1->CreateGameObject(nullptr);
    kible->name = "Kible";
    GameObject* tablicaKibli[8];
    for (int i = 0; i < 8; i++) {
        if (i !=2 && i != 3) {
            tablicaKibli[i] = toiletModel->Instantiate(*scena1, kible, nullptr);
            tablicaKibli[i]->name = "Kibel" + std::to_string(i);
            tablicaKibli[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
            tablicaKibli[i]->AddComponent<ColliderComponent>();
            tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 2.5, 4, 2.5 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->offset       = glm::vec3{ 0, 4, 0 };
            tablicaKibli[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 30, 0.5f, -25 + (-10 * i) };
            tablicaKibli[i]->GetComponent<TransformComponent>()->rotation    = glm::vec3{ 0, 90, 0 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
            tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
        }

        if (i == 2 || i == 3) {
            tablicaKibli[i] = urinModel->Instantiate(*scena1, kible, nullptr);
            tablicaKibli[i]->name = "Kibel" + std::to_string(i);
            tablicaKibli[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 12, 12, 12 };
            tablicaKibli[i]->AddComponent<ColliderComponent>();
            tablicaKibli[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 2.5, 4, 2.5 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->offset       = glm::vec3{ 0, 4, 0 };
            tablicaKibli[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 32.6, 2.0f, -25 + (-10 * i) };
            tablicaKibli[i]->GetComponent<TransformComponent>()->rotation    = glm::vec3{ 0, 270, 0 };
            tablicaKibli[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
            tablicaKibli[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;
        }
    }

    // Zaslony
    GameObject* zaslony = scena1->CreateGameObject(nullptr);
    zaslony->name = "Zasolony";
    GameObject* tablicaZaslon[9];
    for (int i = 0; i < 9; i++) {

        tablicaZaslon[i] = wallModel3->Instantiate(*scena1, zaslony, nullptr);
        tablicaZaslon[i]->GetComponent<TransformComponent>()->scale = glm::vec3{ 0.3, 30, 20 };
        tablicaZaslon[i]->name = "Zaslona" + std::to_string(i);
        tablicaZaslon[i]->AddComponent<ColliderComponent>();
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->halfSize     = glm::vec3{ 20, 15, 0.3 };
        tablicaZaslon[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 35, 0, -20 + (-10 * i) };
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->isWalkable     = false;
        tablicaZaslon[i]->GetComponent<ColliderComponent>()->affectsNavMesh = true;

        if (i==3) {
            tablicaZaslon[i]->GetComponent<TransformComponent>()->position    = glm::vec3{ 5000, 0, -20 + (-10 * i) };
        }
    }

    // Drzwi do kibla
    for (int i = 0; i < 8; i++) {
        if (i != 2 && i != 3) {
            glm::vec3 doorPos      = glm::vec3{ 15.0f, 10.050, -24.65f + (-10.0f * i) };
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
    GameObject* tablicaPokrywek[6];
    for (int i = 0; i < 6; i++) {
        if (i == 0 || i == 3)
            tablicaPapierowKibel[i] = toiletPaperGreenModel->Instantiate(*scena1, nullptr, nullptr);
        if (i == 1 || i == 5)
            tablicaPapierowKibel[i] = toiletPaperRedModel->Instantiate(*scena1, nullptr, nullptr);
        if (i == 2 || i == 4)
            tablicaPapierowKibel[i] = toiletPaperBlueModel->Instantiate(*scena1, nullptr, nullptr);

        tablicaPokrywek[i] = pokrywkaRolkiModel->Instantiate(*scena1, nullptr, nullptr);
        tablicaPokrywek[i]->name = "pokrywkaKibel"+std::to_string(i);
        tablicaPokrywek[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
        tablicaPokrywek[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaPokrywek[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) };

        tablicaPapierowKibel[i]->name = "PapierKibel" + std::to_string(i);
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaPapierowKibel[i]->AddComponent<ColliderComponent>();
        tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) };
        rotatableObjects.insert(tablicaPapierowKibel[i]);
        if (i < 2) {
            tablicaPapierowKibel[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) + 20 };
            tablicaPokrywek[i]->GetComponent<TransformComponent>()->position   = glm::vec3{ 21, 5.0, -40.7 + (-10 * i) + 20 };
        }
    }

    // Zlewy - pozycja X z MainRoomIPoprawkiModeli (-20.5)
    GameObject* zlewy = scena1->CreateGameObject(nullptr);
    zlewy->name = "Zlewy";
    GameObject* tablicaSink[8];
    for (int i = 0; i < 8; i++) {
        tablicaSink[i] = sinkModel->Instantiate(*scena1, zlewy, nullptr);
        tablicaSink[i]->name = "Sink" + std::to_string(i);
        tablicaSink[i]->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
        tablicaSink[i]->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, 90, 0 };
        tablicaSink[i]->GetComponent<TransformComponent>()->position = glm::vec3{ -5.5, 6.0, -20 + (-10 * i) };
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

    GameObject* lustra = scena1->CreateGameObject(nullptr);
    lustra->name = "Lustra";
    // Lustra 1-3
    GameObject* lustro1 = mirrorModel1->Instantiate(*scena1, lustra, nullptr);
    lustro1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro1->AddComponent<ColliderComponent>();
    lustro1->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 0) };

    GameObject* lustro2 = mirrorModel2->Instantiate(*scena1, lustra, nullptr);
    lustro2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro2->AddComponent<ColliderComponent>();
    lustro2->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 1) };

    GameObject* lustro3 = mirrorModel3->Instantiate(*scena1, lustra, nullptr);
    lustro3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro3->AddComponent<ColliderComponent>();
    lustro3->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 2) };

    // Lustro 4 - dodane z mirrorModel4 (lustro_puste.glb)
    GameObject* lustro4 = mirrorModel4->Instantiate(*scena1, lustra, nullptr);
    lustro4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 2, 8 };
    lustro4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0, -180, 0 };
    lustro4->AddComponent<ColliderComponent>();
    lustro4->GetComponent<TransformComponent>()->position   = glm::vec3{ -8.5, 12.0, -25 + (-20 * 3) };

    // Drzwi wyjsciowe z lazienki (washroomExit)
    GameObject* hingeWashroomExit = CreateInteractableDoor(
    scena1, washroomExit.get(), nullptr, "WashroomExit",
    glm::vec3(5.0f, 0.0f, -100.0f),
    glm::vec3{ 10, 11, 10 },
    glm::vec3(5.0f, 0.0f, 0.0f),
    glm::vec3{ 5, 22, 1 },
    90.0f,
    180.0f,
    glm::vec3{ 1, 22, 5 },
    glm::vec3(0.0f, 0.0f, 4.0f)
);
    //majorDoors.insert(hingeWashroomExit);
    toiletDoorsMap[hingeWashroomExit].requiresUnlock = true;
}

void createMainRooom(Scene* scena) {
    // Podloga i sufit
    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaMainRoom", glm::vec3(23.300, 0, -146.390),  glm::vec3(37.070, 1, 43.540));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitMainRoom",   glm::vec3(23.300, 22, -144.490), glm::vec3(37.070, 1, 43.820));

    // Sciany
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaDoRentgenaPrawa",        glm::vec3(35, 0, -203+14),   glm::vec3(25, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "Zauek",        glm::vec3(35, 0, -102.220),   glm::vec3(23.000, 50, 4.270));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaDoRentgenaLewa",         glm::vec3(-10, 0, -203+14),  glm::vec3(10, 50, 1));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaPrawaDoKrematorium",     glm::vec3(60, 0, -105.440),   glm::vec3(4.660, 50, 1), std::nullopt, glm::vec3(1, 50, 4.660));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaLewaDoKrematorium",      glm::vec3(60, 0, -169.010),   glm::vec3(48.650, 50, 1), std::nullopt, glm::vec3(1, 50, 48.650));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaPrawaWRentgenie",      glm::vec3(60, 0, -260.350),   glm::vec3(41.79, 50, 1), std::nullopt, glm::vec3(1, 50, 41.79));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaPrawaDoATOMU",           glm::vec3(-17.000+7, 0, -120.810),  glm::vec3(20.090, 50, 1), std::nullopt, glm::vec3(1, 50, 20.090));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaLewaDoATOMU",            glm::vec3(-17.000+7, 0,  -169.580),  glm::vec3(18.900, 50, 1), std::nullopt, glm::vec3(1, 50, 18.900));

    // Gora przejscia-169.010
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoRentgena",              glm::vec3(0, 66, -189.000),   glm::vec3(10.000, 50, 1));
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoRentgenaZKorytarza",              glm::vec3(-80.620, 66, -152.640),   glm::vec3(10.000, 50, 0.580));
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoKrematoriumZKorytarza",              glm::vec3(132.090-5, 66, -121.670),   glm::vec3(10.000, 50, 0.580));
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoKrematorium",           glm::vec3(60, 66, -209.590),  glm::vec3(100, 50, 1), glm::vec3(0, 90, 0));
    CreateStaticObject(scena, wallModel.get(), nullptr, "GoraPrzejscieDoREAKTORAATOMOWEGO",     glm::vec3(-17.000+7, 66, -209.590), glm::vec3(100, 50, 1), glm::vec3(0, 90, 0));

    glm::vec3 scaleDoors = glm::vec3(2.25, 2.2, 1);

    GameObject* hingeKrematorium = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoKrematorium",
        glm::vec3(59.850f, 7.300f, -115.090+3), glm::vec3(3.600, 3.400, 1),
        glm::vec3(0.0f, 0.0f, -8.0f), glm::vec3(1, 20.0f, 5.7f), -90.0f, 90.0f
    );
    //129.950 115.090
    toiletDoorsMap[hingeKrematorium].canBeClicked = false;
    mainRoomDoors.push_back(hingeKrematorium);

    GameObject* hingeRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoRentgen",
        glm::vec3(5.440f-3, 7.300f, -217.800f+15+14), glm::vec3(3.600, 3.400, 1),
        glm::vec3(8.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), 90.0f, 0.0f,glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );
    /*GameObject* hingeDrzwiDoRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoKrematorium",
        glm::vec3(131.390-5-2-0.3, 7.300f, -121.670), glm::vec3(3.600, 3.400, 1),
        glm::vec3(8.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), -90.0f, 0.0f, glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );*/
    toiletDoorsMap[hingeRentgen].canBeClicked = false;
    mainRoomDoors.push_back(hingeRentgen);

    GameObject* hingeATOM = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoATOMU",
        glm::vec3(-17.000+7, 7.300f, -146.010+3), glm::vec3(3.600, 3.400, 1),
        glm::vec3(0.0f, 0.0f, -8.0f), glm::vec3(0.8f, 20.0f, 5.7f), 90.0f, 90.0f,glm::vec3(4.670, 20.0f, 1.0), glm::vec3(-0.070f, 0.0f, 0), glm::vec3(0, 0.0f, -2)
    );
    toiletDoorsMap[hingeATOM].canBeClicked = false;
    mainRoomDoors.push_back(hingeATOM);

    GameObject* szafkaObj = szafkaModel->Instantiate(*scena, nullptr, nullptr);
    szafkaObj->name = "Szafka";

    TransformComponent* szafkaTr = szafkaObj->GetComponent<TransformComponent>();
    szafkaTr->position = glm::vec3{ 27.0f, 6.530, -216.490f +15+14};
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
    bossCapsule->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    bossCapsule->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    bossCapsule->GetComponent<TransformComponent>()->position = glm::vec3{51.58 ,5, -180.960}; //194
    bossCapsule->AddComponent<ColliderComponent>();
    bossCapsule->GetComponent<ColliderComponent>()->halfSize    = glm::vec3{ 5.34, 5.34f, 5.34f };

    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens1",     glm::vec3(56, 4.8, -176+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens2",     glm::vec3(56, 4.8, -152+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens3",     glm::vec3(20+7, 4.8, -176+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens4",     glm::vec3(20+7, 4.8, -152+15), glm::vec3(8, 8, 8), glm::vec3(0, -90, 0)); // glm::vec3(2, 5, 13)
    GameObject * kredens5 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens5",     glm::vec3(27.7+7, 4.8, -177+15), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0)); //glm::vec3(4, 5, 13)
    GameObject * kredens6 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens6",     glm::vec3(27.7+7, 4.8, -153+15), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0)); //glm::vec3(4, 5, 13)
    GameObject * kredens7 = CreateStaticObject(scena, kredensModel.get(), nullptr, "kredens7",     glm::vec3(-12.230+7, 4.8, -122.540), glm::vec3(8, 8, 8), glm::vec3(0, -270, 0)); //glm::vec3(4, 5, 13)
    //kredens5->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };
    //kredens6->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };
    //kredens7->GetComponent<ColliderComponent>()->offset = glm::vec3{ -2.0f, 0.0f, 0.0f };

    GameObject * eksp1 = eksp1Model->Instantiate(*scena, nullptr, nullptr);
    eksp1->name = "eksp1";
    eksp1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    eksp1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -180.0f, 0.0f };
    eksp1->GetComponent<TransformComponent>()->position = glm::vec3{ 57.170 ,9.510, -165.340 + 15 };

    GameObject * eksp2 = eksp2Model->Instantiate(*scena, nullptr, nullptr);
    eksp2->name = "eksp2";
    eksp2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    eksp2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -111.000f, 0.0f };
    eksp2->GetComponent<TransformComponent>()->position = glm::vec3{ 57.250 ,9.000, -178.730f + 15  };

    GameObject * fiolka1 = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1->name = "fiolka1";
    fiolka1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka1->GetComponent<TransformComponent>()->position = glm::vec3{ 20+7 ,9, -144.64f + 15  };
    GameObject * fiolka1b = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1b->name = "fiolka1b";
    fiolka1b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -300.0f, 0.0f };
    fiolka1b->GetComponent<TransformComponent>()->position = glm::vec3{ 22+7 ,9, -146.64f + 15  };
    GameObject * fiolka1c = fiolka1Model->Instantiate(*scena, nullptr, nullptr);
    fiolka1c->name = "fiolka1c";
    fiolka1c->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    fiolka1c->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -200.0f, 0.0f };
    fiolka1c->GetComponent<TransformComponent>()->position = glm::vec3{ 22+7 ,9, -142.64f + 15  };

    GameObject * fiolka2 = fiolka2Model->Instantiate(*scena, nullptr, nullptr);
    fiolka2->name = "fiolka2";
    fiolka2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10.000, 10.000, 10.000 };
    fiolka2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka2->GetComponent<TransformComponent>()->position = glm::vec3{ 58.270 ,9.000, -174.380f + 15  };
    GameObject * fiolka2b = fiolka2Model->Instantiate(*scena, nullptr, nullptr);
    fiolka2b->name = "fiolka2b";
    fiolka2b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10.000, 10.000, 10.000 };
    fiolka2b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    fiolka2b->GetComponent<TransformComponent>()->position = glm::vec3{ 57.630 ,9.000, -171.140f  + 15 };

    GameObject * ksiazka = ksiazkaModel->Instantiate(*scena, nullptr, nullptr);
    ksiazka->name = "ksiazka";
    ksiazka->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10, 10, 10 };
    ksiazka->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    ksiazka->GetComponent<TransformComponent>()->position = glm::vec3{ 56.920 ,8.560, -150.740  + 15 };

    GameObject* probowki = scena->CreateGameObject(nullptr);
    probowki->name = "probowki";

    GameObject* probowka7 = probowka7Model->Instantiate(*scena, probowki, nullptr);
    probowka7->name = "probowka7";
    probowka7->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka7->GetComponent<TransformComponent>()->position = glm::vec3{ 57.570 ,9, -174.640f  + 15 };
    GameObject * probowka7b = probowka7Model->Instantiate(*scena, probowki, nullptr);
    probowka7b->name = "probowka7b";
    probowka7b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -30.0f, 0.0f };
    probowka7b->GetComponent<TransformComponent>()->position = glm::vec3{ 25.960+7 ,9, -174.640f + 15  };
    GameObject * probowka7c = probowka7Model->Instantiate(*scena, nullptr, nullptr);
    probowka7c->name = "probowka7c";
    probowka7c->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka7c->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 45.0f, 0.0f };
    probowka7c->GetComponent<TransformComponent>()->position = glm::vec3{ 21.360+7 ,9, -182.860  + 15 };

    GameObject * probowka6 = probowka6Model->Instantiate(*scena, probowki, nullptr);
    probowka6->name = "probowka6";
    probowka6->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka6->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -66.900, 0.0f };
    probowka6->GetComponent<TransformComponent>()->position = glm::vec3{ 56.930 ,8.580, -172.450 + 15  };
    GameObject * probowka6b = probowka6Model->Instantiate(*scena, probowki, nullptr);
    probowka6b->name = "probowka6b";
    probowka6b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka6b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -115.900, 0.0f };
    probowka6b->GetComponent<TransformComponent>()->position = glm::vec3{ 56.930 ,8.580, -174.460  + 15 };

    GameObject * probowka5 = probowka5Model->Instantiate(*scena, probowki, nullptr);
    probowka5->name = "probowka5";
    probowka5->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka5->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -45.0f, 0.0f };
    probowka5->GetComponent<TransformComponent>()->position = glm::vec3{ 25.690+7 ,8.570, -144.030  + 15 };
    GameObject * probowka5b = probowka5Model->Instantiate(*scena, nullptr, nullptr);
    probowka5b->name = "probowka5b";
    probowka5b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.5, 1.5, 1.5 };
    probowka5b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 90.0f, -112.0f, 0.0f };
    probowka5b->GetComponent<TransformComponent>()->position = glm::vec3{ 25.880+7 ,8.570, -147.370  + 15 };

    GameObject * probowkaArka_1 = probowkaArka_1_Model->Instantiate(*scena, probowki, nullptr);
    probowkaArka_1->name = "probowkaArka_1";
    probowkaArka_1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowkaArka_1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -11.300, 0.0f };
    probowkaArka_1->GetComponent<TransformComponent>()->position = glm::vec3{ 57.550 ,9.260, -184.430 + 15  };

    GameObject * probowka3 = probowka3Model->Instantiate(*scena, probowki, nullptr);
    probowka3->name = "probowka3";
    probowka3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1.500, 1.500, 1.500 };
    probowka3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -151.800, 0.0f };
    probowka3->GetComponent<TransformComponent>()->position = glm::vec3{ 55.760 ,9.640, -185.700 + 15  };

    GameObject * probowka4 = probowka4Model->Instantiate(*scena, probowki, nullptr);
    probowka4->name = "probowka4";
    probowka4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowka4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka4->GetComponent<TransformComponent>()->position = glm::vec3{ 56.080 ,10.500, -162.990  + 15 };
    GameObject * probowka4b = probowka4Model->Instantiate(*scena, probowki, nullptr);
    probowka4b->name = "probowka4b";
    probowka4b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.250, 0.250, 0.250 };
    probowka4b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    probowka4b->GetComponent<TransformComponent>()->position = glm::vec3{ 57.500 ,10.500, -161.540  + 15 };

    GameObject * labOla1 = labOla1Model->Instantiate(*scena, nullptr, nullptr);
    labOla1->name = "labOla1";
    labOla1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1, 1, 1 };
    labOla1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    labOla1->GetComponent<TransformComponent>()->position = glm::vec3{ 56.840 ,9.540, -157.110 + 15  };

    GameObject * probowka2 = probowka2Model->Instantiate(*scena, nullptr, nullptr);
    probowka2->name = "probowka2";
    probowka2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    probowka2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -176.200, 0.0f };
    probowka2->GetComponent<TransformComponent>()->position = glm::vec3{ 24.200+7 ,9.400, -145.450 + 15  };

    GameObject * folder = folderModel->Instantiate(*scena, nullptr, nullptr);
    folder->name = "folder";
    folder->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    folder->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -93.300, 0.0f };
    folder->GetComponent<TransformComponent>()->position = glm::vec3{ 26.550+7 ,8.390, -150.040  + 15 };

    GameObject * papers = papersModel->Instantiate(*scena, nullptr, nullptr);
    papers->name = "papers";
    papers->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    papers->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    papers->GetComponent<TransformComponent>()->position = glm::vec3{ 22.080+7 ,8.270, -149.310 + 15  };

    GameObject * cup = cupModel->Instantiate(*scena, nullptr, nullptr);
    cup->name = "cup";
    cup->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    cup->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -45.0f, 0.0f };
    cup->GetComponent<TransformComponent>()->position = glm::vec3{ 21.060+7 ,8.420, -186.810 + 15  };
    GameObject * cup2 = cupModel->Instantiate(*scena, nullptr, nullptr);
    cup2->name = "cup2";
    cup2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    cup2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -169.600, 0.0f };
    cup2->GetComponent<TransformComponent>()->position = glm::vec3{ 24.210+7 ,8.420, -185.480  + 15 };

    GameObject * corkBoard = corkBoardModel->Instantiate(*scena, nullptr, nullptr);
    corkBoard->name = "corkBoard";
    corkBoard->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    corkBoard->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -90.000, 0.0f };
    corkBoard->GetComponent<TransformComponent>()->position = glm::vec3{ 58.740 ,13.320, -177.210  + 15 };

    GameObject * clock = clockModel->Instantiate(*scena, nullptr, nullptr);
    clock->name = "clock";
    clock->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    clock->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 180.000, 0.0f };
    clock->GetComponent<TransformComponent>()->position = glm::vec3{ 58.840 ,17.080, -147.150  + 15 };

    GameObject * computer_pbr = computer_pbrModel->Instantiate(*scena, nullptr, nullptr);
    computer_pbr->name = "computer_pbr";
    computer_pbr->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    computer_pbr->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    computer_pbr->GetComponent<TransformComponent>()->position = glm::vec3{ 57.280 ,8.170, -142.870 + 15 };
    GameObject * computer_pbr2 = computer_pbrModel->Instantiate(*scena, nullptr, nullptr);
    computer_pbr2->name = "computer_pbr2";
    computer_pbr2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2, 2, 2 };
    computer_pbr2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
    computer_pbr2->GetComponent<TransformComponent>()->position = glm::vec3{ 57.280 ,8.170, -147.460 + 15 };

    GameObject * laboratoryStuff1 = laboratoryStuff1Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff1->name = "laboratoryStuff1";
    laboratoryStuff1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    laboratoryStuff1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -123.800, 0.0f };
    laboratoryStuff1->GetComponent<TransformComponent>()->position = glm::vec3{ 27.760 ,11.070, -157.870 + 15  };

    GameObject * laboratoryStuff2 = laboratoryStuff2Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff2->name = "laboratoryStuff2";
    laboratoryStuff2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 2.5, 2.5, 2.5 };
    laboratoryStuff2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -72.300, 0.0f };
    laboratoryStuff2->GetComponent<TransformComponent>()->position = glm::vec3{ 23.630+7 ,10.250, -165.330 + 15  };

    GameObject * laboratoryStuff3 = laboratoryStuff3Model->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff3->name = "laboratoryStuff3";
    laboratoryStuff3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 3, 3, 3 };
    laboratoryStuff3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 7.500, 0.0f };
    laboratoryStuff3->GetComponent<TransformComponent>()->position = glm::vec3{ 23.540+7 ,8.540, -178.320  + 15 };

    GameObject * krzeslo1 = krzesloModel->Instantiate(*scena, nullptr, nullptr);
    krzeslo1->name = "krzeslo1";
    krzeslo1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    krzeslo1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 159.200, 0.0f };
    krzeslo1->GetComponent<TransformComponent>()->position = glm::vec3{ 48.160 ,4.590, -126.720 };
    krzeslo1->AddComponent<ColliderComponent>();
    GameObject * krzeslo2 = krzesloModel->Instantiate(*scena, nullptr, nullptr);
    laboratoryStuff3->name = "krzeslo2";
    krzeslo2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    krzeslo2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 106.100, 0.0f };
    krzeslo2->GetComponent<TransformComponent>()->position = glm::vec3{ 48.160 ,4.590, -139.860};
    krzeslo2->AddComponent<ColliderComponent>();
    GameObject * Szafka_lab1 = szafka_labModel->Instantiate(*scena, nullptr, nullptr);
    Szafka_lab1->name = "Szafka_lab1";
    Szafka_lab1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10 };
    Szafka_lab1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 91.000, 0.0f };
    Szafka_lab1->GetComponent<TransformComponent>()->position = glm::vec3{ -13.160+7 ,6.680, -190.470+14};
    Szafka_lab1->AddComponent<ColliderComponent>();
    Szafka_lab1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 5.0f, 5, 10.0f };
    GameObject * Szafka_lab2 = szafka_labModel->Instantiate(*scena, nullptr, nullptr);
    Szafka_lab2->name = "Szafka_lab2";
    Szafka_lab2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10 };
    Szafka_lab2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 88, 0.0f };
    Szafka_lab2->GetComponent<TransformComponent>()->position = glm::vec3{ -13.160+7 ,6.680, -176.930+14};
    Szafka_lab2->AddComponent<ColliderComponent>();
    Szafka_lab2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 5.0f, 5, 10.0f };
    if (cabState.button) {
        ColliderComponent* btnCol = cabState.button->AddComponent<ColliderComponent>();
        btnCol->halfSize = glm::vec3{ 1.0f, 1.0f, 1.0f };

        TransformComponent* btnTr = cabState.button->GetComponent<TransformComponent>();
        cabState.buttonStartPos  = btnTr->position;
        cabState.buttonTargetPos = cabState.buttonStartPos + glm::vec3{ 0.0f, 0.0f, -0.15f };

        cabinetsMap[cabState.button] = cabState;
    }


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
    bossTr->position = glm::vec3(52.590, 7.320, -196.120+14);
    bossTr->scale = glm::vec3(1.5f);
    bossTr->rotation = glm::vec3(0.0f, 130.000, 0.0f);
    bossTr->isDirty = true;

    AnimatorComponent* bossAnimator = bossObj->AddComponent<AnimatorComponent>();

    if (bossModel->rootModel && !bossModel->rootModel->animations.empty()) {
        AnimationClip* defaultBossClip = &bossModel->rootModel->animations[0];

        AnimationHelper::Play(bossAnimator, defaultBossClip, true, 1.0f);
    }

    /*GameObject * szafka_inna1 = szafka_inna1Model->Instantiate(*scena, nullptr, nullptr);
    szafka_inna1->name = "szafka_inna1a";
    szafka_inna1->GetComponent<TransformComponent>()->scale    = glm::vec3{ 6 };
    szafka_inna1->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0, 0.0f };
    szafka_inna1->GetComponent<TransformComponent>()->position = glm::vec3{ 53.500 ,4.120, -103.230};
    szafka_inna1->AddComponent<ColliderComponent>();
    szafka_inna1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 4.520, 5, 2.420f };
    szafka_inna1->GetComponent<ColliderComponent>()->offset = glm::vec3{ 3.210, 0, 0 };*/
    GameObject * szafka_inna1b = szafka_inna1Model->Instantiate(*scena, nullptr, nullptr);
    szafka_inna1b->name = "szafka_inna1b";
    szafka_inna1b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    szafka_inna1b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 180, 0.0f };
    szafka_inna1b->GetComponent<TransformComponent>()->position = glm::vec3{ -9.520+7 ,4.660, -107.320};
    szafka_inna1b->AddComponent<ColliderComponent>();
    szafka_inna1b->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 4.520, 5, 2.420f };
    szafka_inna1b->GetComponent<ColliderComponent>()->offset = glm::vec3{ -3.430, 0, 0 };
    GameObject * szafka_inna2 = szafka_inna2Model->Instantiate(*scena, nullptr, nullptr);
    szafka_inna2->name = "szafka_inna2";
    szafka_inna2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    szafka_inna2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90, 0.0f };
    szafka_inna2->GetComponent<TransformComponent>()->position = glm::vec3{ -8.820+7 ,4.550, -102.990};
    szafka_inna2->AddComponent<ColliderComponent>();
    szafka_inna2->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 5.910, 4.100, 2.760 };
    szafka_inna2->GetComponent<ColliderComponent>()->offset = glm::vec3{ -3.470, 0, 0.0f };
    GameObject * wozek = wozekModel->Instantiate(*scena, nullptr, nullptr);
    wozek->name = "wozek";
    wozek->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7 };
    wozek->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 122.800, 0.0f };
    wozek->GetComponent<TransformComponent>()->position = glm::vec3{ 6.630+7 ,4.800, -145.800};
    wozek->AddComponent<ColliderComponent>();
    wozek->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 4.0f, 5, 5.130 };

    GameObject * eksperyment3 = eksp3Model->Instantiate(*scena, nullptr, nullptr);
    eksperyment3->name = "eksperyment3";
    eksperyment3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 5 };
    eksperyment3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90.100, 0.0f };
    eksperyment3->GetComponent<TransformComponent>()->position = glm::vec3{ -6.030 ,9.180, -112.190};
    GameObject * eksperyment3b = eksp3Model->Instantiate(*scena, nullptr, nullptr);
    eksperyment3b->name = "eksperyment3b";
    eksperyment3b->GetComponent<TransformComponent>()->scale    = glm::vec3{ 5 };
    eksperyment3b->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 102.700, 0.0f };
    eksperyment3b->GetComponent<TransformComponent>()->position = glm::vec3{ -11.400+7 ,9.860, -107.600};
    for (int i = 0; i < 6; i++) {
        GameObject * ekperyment4 = eksp4Model->Instantiate(*scena, nullptr, nullptr);
        ekperyment4->name = "ekperyment4_a" + std::to_string(i);
        ekperyment4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7};
        ekperyment4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0, 0.0f };
        ekperyment4->GetComponent<TransformComponent>()->position = glm::vec3{ -7.910 ,7.760, -157.800 + (-i * 4.5)};
    }
    for (int i = 0; i < 6; i++) {
        GameObject * ekperyment4 = eksp4Model->Instantiate(*scena, nullptr, nullptr);
        ekperyment4->name = "ekperyment4_b" + std::to_string(i);
        ekperyment4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 7};
        ekperyment4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 0, 0.0f };
        ekperyment4->GetComponent<TransformComponent>()->position = glm::vec3{ -5.000 ,7.760, -157.800 + (-i * 4.5)};
    }
    GameObject * fiolka_nast = fiolka_nastModel->Instantiate(*scena, nullptr, nullptr);
    fiolka_nast->name = "fiolka_nast";
    fiolka_nast->GetComponent<TransformComponent>()->scale    = glm::vec3{ 10 };
    fiolka_nast->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90, 0.0f };
    fiolka_nast->GetComponent<TransformComponent>()->position = glm::vec3{ 6.910+7 ,7.650, -145.210};

    GameObject * probowka7d = probowka7Model->Instantiate(*scena, wozek, nullptr);
    probowka7d->name = "probowka7d";
    probowka7d->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25};
    probowka7d->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90.000f, 90.0f };
    probowka7d->GetComponent<TransformComponent>()->position = glm::vec3{ 0.150,0.370,0};
    GameObject * probowka7e = probowka7Model->Instantiate(*scena, wozek, nullptr);
    probowka7e->name = "probowka7e";
    probowka7e->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25};
    probowka7e->GetComponent<TransformComponent>()->rotation = glm::vec3{ 9.300f, 22.400, 90.0f };
    probowka7e->GetComponent<TransformComponent>()->position = glm::vec3{ -0.080,0.370,0.120};

    GameObject * papers2 = papersModel->Instantiate(*scena, wozek, nullptr);
    papers2->name = "papers2";
    papers2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 30.0f, 0.0f };
    papers2->GetComponent<TransformComponent>()->position = glm::vec3{ 0, 0, 0  };

    GameObject * fiolka_nastb = fiolka_nastModel->Instantiate(*scena, kredens7 , nullptr);
    fiolka_nastb->name = "fiolka_nastb";
    fiolka_nastb->GetComponent<TransformComponent>()->scale    = glm::vec3{ 1 };
    fiolka_nastb->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 90, 0.0f };
    fiolka_nastb->GetComponent<TransformComponent>()->position = glm::vec3{ 0 ,0.540, 0};

    GameObject * papers3 = papersModel->Instantiate(*scena, kredens7, nullptr);
    papers3->name = "papers3";
    papers3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 30.0f, 0.0f };
    papers3->GetComponent<TransformComponent>()->position = glm::vec3{ -0.640, 0.450, 0  };
    GameObject * papers4 = papersModel->Instantiate(*scena, kredens7, nullptr);
    papers4->name = "papers4";
    papers4->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers4->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, 245.0f, 0.0f };
    papers4->GetComponent<TransformComponent>()->position = glm::vec3{ -0.220, 0.450, 0  };
    GameObject * papers5 = papersModel->Instantiate(*scena, kredens7, nullptr);
    papers5->name = "papers5";
    papers5->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers5->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    papers5->GetComponent<TransformComponent>()->position = glm::vec3{ 0.270, 0.450, -0.1  };
    GameObject * telephone = telephoneModel->Instantiate(*scena, szafka_inna2, nullptr);
    telephone->name = "telephone";
    telephone->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    telephone->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -151.800, 0.0f };
    telephone->GetComponent<TransformComponent>()->position = glm::vec3{ 0.120, 0.630, -0.350  };
    GameObject * papers6 = papersModel->Instantiate(*scena, szafka_inna2, nullptr);
    papers6->name = "papers6";
    papers6->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers6->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    papers6->GetComponent<TransformComponent>()->position = glm::vec3{ 0.030, 0.650, -0.770  };
    GameObject * papers7 = papersModel->Instantiate(*scena, szafka_inna2, nullptr);
    papers7->name = "papers7";
    papers7->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    papers7->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    papers7->GetComponent<TransformComponent>()->position = glm::vec3{ 0.320, 0.640, -0.680  };
    GameObject * folder2 = folderModel->Instantiate(*scena, szafka_inna2, nullptr);
    folder2->name = "folder2";
    folder2->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    folder2->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    folder2->GetComponent<TransformComponent>()->position = glm::vec3{ 0.320, 0.710, -0.720  };
    GameObject * folder3 = folderModel->Instantiate(*scena, szafka_inna2, nullptr);
    folder3->name = "folder3";
    folder3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    folder3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    folder3->GetComponent<TransformComponent>()->position = glm::vec3{ 1.060, 0.550, -0.430  };
    GameObject * cup3 = cupModel->Instantiate(*scena, szafka_inna2, nullptr);
    cup3->name = "cup3";
    cup3->GetComponent<TransformComponent>()->scale    = glm::vec3{ 0.25 };
    cup3->GetComponent<TransformComponent>()->rotation = glm::vec3{ 0.0f, -55.500, 0.0f };
    cup3->GetComponent<TransformComponent>()->position = glm::vec3{ 0.460, 0.630, -0.210  };
}

void createNuclearRooom(Scene* scena) {
    //CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaNucearRoom", glm::vec3(-120, 0, -180),  glm::vec3(60, 1, 80));
    //CreateStaticObject(scena, floorModel.get(), nullptr, "SufitATOM",         glm::vec3(-120, 20, -180), glm::vec3(60, 1, 80));
    //CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaKoncowaAtom", glm::vec3(-180, 0, -180),  glm::vec3(80, 50, 1), std::nullopt, glm::vec3(1, 50, 80));
    //CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaATOMPrawa",   glm::vec3(-120.180, 0, -259.680), glm::vec3(60, 50, 1), std::nullopt, glm::vec3(60, 100, 1));
}

void createCrematorium(Scene* scena) {

    /*
    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaKrematorium",   glm::vec3(120, 0, -180),  glm::vec3(60, 1, 80));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitCrematorium",     glm::vec3(120, 25, -180), glm::vec3(60, 1, 80));
    CreateStaticObject(scena, wallModel2.get(), nullptr, "ScianaKoncowaKrematorium", glm::vec3(180, 0, -180), glm::vec3(80, 50, 1), std::nullopt, glm::vec3(1, 50, 80));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaKremLewa",       glm::vec3(120.180, 0, -259.680), glm::vec3(60, 50, 1), std::nullopt, glm::vec3(60, 100, 1));
    */

    crematoriumPuzzle.spacingHorizontal = 10.0f;
    crematoriumPuzzle.spacingVertical   = 6.0f;

    crematoriumPuzzle.minExtensionDistance = 15.0f;
    crematoriumPuzzle.maxExtensionDistance = 45.0f;

    crematoriumPuzzle.coffinDimensions = glm::vec3(1.25f, 1.0f, 31.0f);

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
        spdlog::error("Model rury albo panelu nie zostal poprawnie zaladowany!");
    }
}

void createRentgenRoom(Scene* scena) {
    GameObject * podlogaRentgenRoom =CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaRentgenRoom",    glm::vec3(-84.700+10, 0, -183.720),  glm::vec3(30.000, 1, 30.000));
    GameObject * sufitRentgen = CreateStaticObject(scena, floorModel.get(), nullptr, "SufitRentgen",          glm::vec3(-84.700+10, 17, -183.720), glm::vec3(30, 1, 30));
    GameObject * KoncowaScianaRentgen = CreateStaticObject(scena, wallModel.get(),  nullptr, "KoncowaScianaRentgen",  glm::vec3(-84.570+10, 0, -214.550+10),          glm::vec3(32, 50, 1));
    GameObject* ScianaRentgen2 = CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaRentgen2",         glm::vec3(-102.430, 0, -153.020),  glm::vec3(12.600, 50, 1));
    GameObject * LewaScianaRentgen = CreateStaticObject(scena, wallModel.get(),  nullptr, "LewaScianaRentgen",    glm::vec3(-105.590+10, 0.000, -183.580),  glm::vec3(32.000, 50.000, 1), glm::vec3(180, 90, 180), glm::vec3(1,50.000,32.000), true);
    GameObject * PrawaScianaRentgen = CreateStaticObject(scena, wallModel.get(),  nullptr, "PrawaScianaRentgen",    glm::vec3(-54.970+10, 0.000, -183.580),  glm::vec3(32.000, 50.000, 1), glm::vec3(180, 90, 180), glm::vec3(1,50.000,32.000), true);
    GameObject * objPuzel1 = puzel1->Instantiate(*scena, nullptr, nullptr);
    objPuzel1->name = "puzel1";
    objPuzel1->GetComponent<TransformComponent>()->position = glm::vec3(-91.660, 6.250, -185.175);
    objPuzel1->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel1->AddComponent<RigidbodyComponent>();
    objPuzel1->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel1->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel1->AddComponent<ColliderComponent>();
    objPuzel1->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalRotations[objPuzel1] = objPuzel1->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel1);

    GameObject * objPuzel2 = puzel2->Instantiate(*scena, nullptr, nullptr);
    objPuzel2->name = "puzel2";
    objPuzel2->GetComponent<TransformComponent>()->position = glm::vec3(57.112, 8.537, -167.965);
    objPuzel2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel2->AddComponent<RigidbodyComponent>();
    objPuzel2->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel2->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel2->AddComponent<ColliderComponent>();
    objPuzel2->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalRotations[objPuzel2] = objPuzel2->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel2);

    GameObject * objPuzel3 = puzel3->Instantiate(*scena, nullptr, nullptr);
    objPuzel3->name = "puzel3";
    objPuzel3->GetComponent<TransformComponent>()->position = glm::vec3(0, 1, 0);
    objPuzel3->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel3->AddComponent<RigidbodyComponent>();
    objPuzel3->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel3->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel3->AddComponent<ColliderComponent>();
    objPuzel3->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalRotations[objPuzel3] = objPuzel3->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel3);

    GameObject * objPuzel4 = puzel4->Instantiate(*scena, nullptr, nullptr);
    objPuzel4->name = "puzel4";
    objPuzel4->GetComponent<TransformComponent>()->position = glm::vec3(0, 1, 0);
    objPuzel4->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel4->AddComponent<RigidbodyComponent>();
    objPuzel4->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel4->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel4->AddComponent<ColliderComponent>();
    objPuzel4->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalRotations[objPuzel4] = objPuzel4->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel4);

    GameObject * objPuzel5 = puzel5->Instantiate(*scena, nullptr, nullptr);
    objPuzel5->name = "puzel5";
    objPuzel5->GetComponent<TransformComponent>()->position = glm::vec3(-69.503, 1, -188.319);
    objPuzel5->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel5->AddComponent<RigidbodyComponent>();
    objPuzel5->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel5->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel5->AddComponent<ColliderComponent>();
    objPuzel5->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalRotations[objPuzel5] = objPuzel5->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel5);

    GameObject * objPuzel6 = puzel6->Instantiate(*scena, nullptr, nullptr);
    objPuzel6->name = "puzel6";
    objPuzel6->GetComponent<TransformComponent>()->position = glm::vec3(-7.130, 9.537, -130.855);
    objPuzel6->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 90);
    objPuzel6->AddComponent<RigidbodyComponent>();
    objPuzel6->GetComponent<RigidbodyComponent>()->useGravity = true;
    objPuzel6->GetComponent<RigidbodyComponent>()->isStatic = false;
    objPuzel6->AddComponent<ColliderComponent>();
    objPuzel6->GetComponent<ColliderComponent>()->halfSize = glm::vec3(0.528,0.100,0.834);
    objectOriginalRotations[objPuzel6] = objPuzel6->GetComponent<TransformComponent>()->rotation;
    pickupObjects.insert(objPuzel6);

    GameObject * rentgen = Rentgen->Instantiate(*scena, nullptr, nullptr);
    rentgen->name = "RentgenTablica";
    rentgen->GetComponent<TransformComponent>()->position =glm::vec3(-83.319+10, 10.160, -213.257+10);
    rentgen->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    rentgen->GetComponent<TransformComponent>()->scale = glm::vec3(1.5);
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
        col->halfSize  = glm::vec3(0.840, 1.390, 0.330);
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

    createPuzzleSlot(glm::vec3(-85.052+10, 11.430, -212.70+10), glm::vec3(0, -90,  -0), objPuzel2);
    createPuzzleSlot(glm::vec3(-83.382+10, 11.430, -212.70+10), glm::vec3(0, -90, 0), objPuzel6);
    createPuzzleSlot(glm::vec3(-81.820+10, 11.430, -212.70+10), glm::vec3(0, -90, 0), objPuzel4);
    createPuzzleSlot(glm::vec3(-85.052+10, 9, -212.70+10), glm::vec3(0, -90, 0), objPuzel1);
    createPuzzleSlot(glm::vec3(-83.382+10, 9, -212.70+10), glm::vec3(0, -90, 0), objPuzel3);
    createPuzzleSlot(glm::vec3(-81.820+10, 9, -212.70+10), glm::vec3(0, -90, 0), objPuzel5);

    //-85.197, 6.250, -182.170
    GameObject * lampaOperacyjna = lampaOperacyjnaModel->Instantiate(*scena, nullptr, nullptr);
    lampaOperacyjna->name = "lampaOperacyjna";
    lampaOperacyjna->GetComponent<TransformComponent>()->position =glm::vec3(-51.070, 9.490, -164.000);
    lampaOperacyjna->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 361.400, 0);
    lampaOperacyjna->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    lampaOperacyjna->AddComponent<ColliderComponent>();
    GameObject * stolOperacyjny = stolOperacyjnyModel->Instantiate(*scena, nullptr, nullptr);
    stolOperacyjny->name = "stolOperacyjny";
    stolOperacyjny->GetComponent<TransformComponent>()->position =glm::vec3(-55.527, 3.340, -166.800);
    stolOperacyjny->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -209.800, 0);
    stolOperacyjny->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    stolOperacyjny->AddComponent<ColliderComponent>();
    GameObject * szafka1Rentgen = SzafkaRentgen1Model->Instantiate(*scena, nullptr, nullptr);
    szafka1Rentgen->name = "szafka1Rentgen";
    szafka1Rentgen->GetComponent<TransformComponent>()->position =glm::vec3(-49.367, 8.240, -210.290+10);
    szafka1Rentgen->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -43.900, 0);
    szafka1Rentgen->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    szafka1Rentgen->AddComponent<ColliderComponent>();
    GameObject * szafka1Rentgenb = SzafkaRentgen1Model->Instantiate(*scena, nullptr, nullptr);
    szafka1Rentgenb->name = "szafka1Rentgenb";
    szafka1Rentgenb->GetComponent<TransformComponent>()->position =glm::vec3(-47.327, 8.040, -197.000+10);
    szafka1Rentgenb->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -89.000, 0);
    szafka1Rentgenb->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    szafka1Rentgenb->AddComponent<ColliderComponent>();/*
    GameObject * szafka1Rentgenc = SzafkaRentgen1Model->Instantiate(*scena, nullptr, nullptr);
    szafka1Rentgenc->name = "szafka1Rentgenc";
    szafka1Rentgenc->GetComponent<TransformComponent>()->position =glm::vec3(-89.427, 4.890, -155.840);
    szafka1Rentgenc->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 180.000, 0);
    szafka1Rentgenc->GetComponent<TransformComponent>()->scale = glm::vec3(12.000, 4, 8.500);
    szafka1Rentgenc->AddComponent<ColliderComponent>();*/
    GameObject * szafka2Rentgen = SzafkaRentgen2Model->Instantiate(*scena, nullptr, nullptr);
    szafka2Rentgen->name = "szafka2Rentgen";
    szafka2Rentgen->GetComponent<TransformComponent>()->position =glm::vec3(-53.037, 7.900, -202.100+10);
    szafka2Rentgen->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90.000, 0);
    szafka2Rentgen->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    szafka2Rentgen->AddComponent<ColliderComponent>();
    /*GameObject * szafka2Rentgenb = SzafkaRentgen2Model->Instantiate(*scena, nullptr, nullptr);
    szafka2Rentgenb->name = "szafka2Rentgenb";
    szafka2Rentgenb->GetComponent<TransformComponent>()->position =glm::vec3(-97.477, 8.860, -165.050);
    szafka2Rentgenb->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 95.400, 0);
    szafka2Rentgenb->GetComponent<TransformComponent>()->scale = glm::vec3(8);
    szafka2Rentgenb->AddComponent<ColliderComponent>();
    GameObject * szafka2Rentgenc = SzafkaRentgen2Model->Instantiate(*scena, nullptr, nullptr);
    szafka2Rentgenc->name = "szafka2Rentgenc";
    szafka2Rentgenc->GetComponent<TransformComponent>()->position =glm::vec3(-94.577, 7.990, -205.990);
    szafka2Rentgenc->GetComponent<TransformComponent>()->rotation = glm::vec3(0,0 , 0);
    szafka2Rentgenc->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    szafka2Rentgenc->AddComponent<ColliderComponent>();
    */
    GameObject * zaslona = zaslonaModel->Instantiate(*scena, nullptr, nullptr);
    zaslona->name = "zaslona";
    zaslona->GetComponent<TransformComponent>()->position =glm::vec3(-62.397, 5.910, -172.520);
    zaslona->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 236.500, 0);
    zaslona->GetComponent<TransformComponent>()->scale = glm::vec3(6);
    zaslona->AddComponent<ColliderComponent>();
    GameObject * zaslonab = zaslonaModel->Instantiate(*scena, nullptr, nullptr);
    zaslonab->name = "zaslonab";
    zaslonab->GetComponent<TransformComponent>()->position =glm::vec3(-58.937, 5.910, -174.740);
    zaslonab->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 207.700, 0);
    zaslonab->GetComponent<TransformComponent>()->scale = glm::vec3(6);
    zaslonab->AddComponent<ColliderComponent>();
    GameObject * zaslonac = zaslonaModel->Instantiate(*scena, nullptr, nullptr);
    zaslonac->name = "zaslonac";
    zaslonac->GetComponent<TransformComponent>()->position =glm::vec3(-64.397, 5.910, -168.250);
    zaslonac->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 255.500, 0);
    zaslonac->GetComponent<TransformComponent>()->scale = glm::vec3(6);
    zaslonac->AddComponent<ColliderComponent>();
    //-73.385, 6.250 -175.377
    GameObject * wozek = wozekModel->Instantiate(*scena, nullptr, nullptr);
    wozek->name = "wozek";
    wozek->GetComponent<TransformComponent>()->position =glm::vec3(-61.165, 4.310, -185.587);
    wozek->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -31.900, 0);
    wozek->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    wozek->AddComponent<ColliderComponent>();
    GameObject * wozek2 = wozekModel->Instantiate(*scena, nullptr, nullptr);
    wozek2->name = "wozek2";
    wozek2->GetComponent<TransformComponent>()->position =glm::vec3(-63.885, 4.310, -156.157);
    wozek2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 0, 0);
    wozek2->GetComponent<TransformComponent>()->scale = glm::vec3(7);
    wozek2->AddComponent<ColliderComponent>();

    GameObject * desk = deskModel->Instantiate(*scena, nullptr, nullptr);
    desk->name = "desk";
    desk->GetComponent<TransformComponent>()->position =glm::vec3(-73.025, 0.860, -200.417);
    desk->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90.000, 0);
    desk->GetComponent<TransformComponent>()->scale = glm::vec3(3);
    desk->AddComponent<ColliderComponent>();

    GameObject * sink = sinkModel->Instantiate(*scena, nullptr, nullptr);
    sink->name = "sink";
    sink->GetComponent<TransformComponent>()->position =glm::vec3(-87.185, 6.250, -201.537);
    sink->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 0, 0);
    sink->GetComponent<TransformComponent>()->scale = glm::vec3(2);
    sink->AddComponent<ColliderComponent>();

    GameObject * drawer1 = drawer1Model->Instantiate(*scena, nullptr, nullptr);
    drawer1->name = "drawer1";
    drawer1->GetComponent<TransformComponent>()->position =glm::vec3(-92.145, 12.860, -199.387);
    drawer1->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    drawer1->GetComponent<TransformComponent>()->scale = glm::vec3(3);
    drawer1->AddComponent<ColliderComponent>();

    GameObject * drawer2 = drawer2Model->Instantiate(*scena, nullptr, nullptr);
    drawer2->name = "drawer2";
    drawer2->GetComponent<TransformComponent>()->position =glm::vec3(-87.355, 12.860, -199.637);
    drawer2->GetComponent<TransformComponent>()->rotation = glm::vec3(0, -90, 0);
    drawer2->GetComponent<TransformComponent>()->scale = glm::vec3(3);
    drawer2->AddComponent<ColliderComponent>();

    GameObject * stol = tableModel->Instantiate(*scena, nullptr, nullptr);
    stol->name = "stol";
    stol->GetComponent<TransformComponent>()->position =glm::vec3(-89.965, 3.100, -162.607);
    stol->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 0, 0);
    stol->GetComponent<TransformComponent>()->scale = glm::vec3(8.000);
    stol->AddComponent<ColliderComponent>();

    GameObject * kredens = kredensModel->Instantiate(*scena, nullptr, nullptr);
    kredens->name = "kredens";
    kredens->GetComponent<TransformComponent>()->position =glm::vec3(-91.795, 3.840, -182.037);
    kredens->GetComponent<TransformComponent>()->rotation = glm::vec3(0, 90, 0);
    kredens->GetComponent<TransformComponent>()->scale = glm::vec3(6);
    kredens->AddComponent<ColliderComponent>();

}
void createRentgenCorridor(Scene * scena){
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaLewaKorytarzRentgen",         glm::vec3(-50.470, 0, -137.540),  glm::vec3(41.440, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaPrawaKorytarzRentgen1",         glm::vec3(-86.440, 0, -153.020),  glm::vec3(4.630, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaPrawaKorytarzRentgen2",         glm::vec3(-41.200, 0, -153.020),  glm::vec3(30.460, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaKoncowaKorytarzRentgen",         glm::vec3(-91.890, 0, -144.780),  glm::vec3(1, 90, 7.870));
    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaKorytarzRentgen",    glm::vec3(-52.440, 0, -145.900),  glm::vec3(43.520, 1, 8.070));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitKorytarzRentgen",    glm::vec3(-52.440, 20, -145.900),  glm::vec3(43.520, 1, 8.070));
    GameObject* hingeDrzwiDoRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoRentgenZKorytarza",
        glm::vec3(-79, 7.300f, -152.310), glm::vec3(3.600, 3.400, 1),
        glm::vec3(8.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), -90.0f, 0.0f, glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );
    unlockedDoors.insert(hingeDrzwiDoRentgen);

}

void createCrematoriumCorridor(Scene * scena){
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaLewaKorytarzKrematorium",         glm::vec3(102.160, 0, -107.230),  glm::vec3(41.440, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaPrawaKorytarzKrematorium1",         glm::vec3(135.720, 0, -121.660),  glm::vec3(4.630, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaPrawaKorytarzKrematorium2",         glm::vec3(90.570, 0, -121.660),  glm::vec3(30.460, 50, 1));
    CreateStaticObject(scena, wallModel.get(),  nullptr, "ScianaKoncowaKorytarzKrematorium",         glm::vec3(141.810, 0, -113.790),  glm::vec3(1, 90, 7.870));
    CreateStaticObject(scena, floorModel.get(), nullptr, "PodlogaKorytarzKrematorium",    glm::vec3(102.610, 0, -115.010),  glm::vec3(43.520, 1, 8.070));
    CreateStaticObject(scena, floorModel.get(), nullptr, "SufitKorytarzKrematorium",    glm::vec3(102.610, 20, -115.010),  glm::vec3(43.520, 1, 8.070));
    GameObject* hingeDrzwiDoRentgen = CreateInteractableDoor(
        scena, NormalDoor.get(), nullptr, "DrzwiDoKrematorium",
        glm::vec3(131.390-5-2-0.3, 7.300f, -121.670), glm::vec3(3.600, 3.400, 1),
        glm::vec3(8.0f, 0.0f, 0.0f), glm::vec3(5.7f, 20.0f, 1.0f), -90.0f, 0.0f, glm::vec3(1.0f, 20.0f, 4.670), glm::vec3(0.0f, 0.0f, -5.070), glm::vec3(2, 0.0f, 0.0f)
    );
    unlockedDoors.insert(hingeDrzwiDoRentgen);
}
#include "impl/main/room_creator.ipp"
