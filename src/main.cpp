// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#include <stdio.h>

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
#include <entity.h>
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
#include <systems/SpriteSystem.h>

#include "diagnostics/cpu_timer.h"
#include "utils/render_helper.h"


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool init();
void init_imgui();


void compileShader();

void input();
void controlKoparka();
void update();
//void render();
void renderInstanced(Model* model, Shader* shader, std::vector<Entity*>& entities);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadCubemap(vector<std::string> faces);
void regenerateSphere();

//void createHouse();
void processCameraInput(ECS& ecs, CameraComponent& cam,
    const std::string& up,
    const std::string& down,
    const std::string& left,
    const std::string& right);


void imgui_begin();
void imgui_render();
void imgui_end();

void end_frame();

constexpr int32_t WINDOW_WIDTH = 1600;
constexpr int32_t WINDOW_HEIGHT = 900;

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


//std::unique_ptr<Model> sphereVenusModel;
//
std::unique_ptr<Prefab> wallModel;
std::unique_ptr<Prefab> roofModel;
std::unique_ptr<Prefab> groundModel;

std::unique_ptr<Prefab> koparkaModel;


std::unique_ptr<Entity> root;
Entity* venus;
Entity* koparkaEntity;

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


void updateFPS(float deltaTime) {
    frameTimes[index] = deltaTime;
    index = (index + 1) % MAX_SAMPLES;

    float sum = 0.0f;
    for (int i = 0; i < MAX_SAMPLES; i++)
        sum += frameTimes[i];

    float avg = sum / MAX_SAMPLES;
    float fps = 1.0f / avg;

    spdlog::info("FPS: {}", fps);
}

void updateFocus() {
    if (focused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    if (!focused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

}


void processCameraInput(ECS& ecs,CameraComponent& cam, TransformComponent& transform,
                       const std::string& up,
                       const std::string& down,
                       const std::string& left,
                       const std::string& right)
{
    const auto& hid = ecs.GetSystem<HID>();
    glm::vec3 dir(0.0f);

    if (hid->is_action_pressed(up))    dir += cam.state.Front;
    if (hid->is_action_pressed(left))  dir -= cam.state.Right;
    if (hid->is_action_pressed(right)) dir += cam.state.Right;
    if (hid->is_action_pressed(down))  dir -= cam.state.Front;

    if (glm::length(dir) > 0.0f)
        dir = glm::normalize(dir);

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        transform.position += dir * MovementSpeed * deltaTime;
    }
}

void processCameraMouse(ECS& ecs, CameraComponent& cam)
{
    const auto& hid = ecs.GetSystem<HID>();
    CameraHelper::ProcessMouseMovement(cam,
        (float)hid->get_mouse_dx(),
        (float)-hid->get_mouse_dy()
    );
}

void processCameraGamepad(ECS& ecs,
                         CameraComponent& cam,
                         TransformComponent& transform,
                         int gamepad_id);

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


    ecs.AddSystem<TransformSystem>(ecs);
    ecs.AddSystem<PhysicsSystem>(ecs);
    ecs.AddSystem<RenderSystem>(ecs, window);
    ecs.AddSystem<HID>(ecs, window);
    ecs.AddSystem<SpriteSystem>(ecs, window);

    ourShader = std::make_unique<Shader>("res/shaders/basic.vert", "res/shaders/basic.frag");
    ourShader->use();

    groundModel = std::make_unique<Prefab>("res/models/podloze.glb");
    sunModel = std::make_unique<Prefab>("res/models/Sun.glb");
    GameObject* obb = groundModel->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* obb2 = sunModel->Instantiate(*scena1, nullptr, ourShader.get());
 

    obb->GetComponent<TransformComponent>()->scale.x = 1;
    obb->GetComponent<TransformComponent>()->scale.y = 1;
    obb->GetComponent<TransformComponent>()->scale.z = 1;


   //obb2->GetComponent<TransformComponent>()->scale.x = 25;
    //obb2->GetComponent<TransformComponent>()->scale.y = 25;
   //obb2->GetComponent<TransformComponent>()->scale.z = 25;
   //obb2->GetComponent<TransformComponent>()->position.y = 250;

    obb->AddComponent<RigidbodyComponent>();
    obb->AddComponent<ColliderComponent>();



    obb->GetComponent<RigidbodyComponent>()->useGravity = false;
    obb->GetComponent<RigidbodyComponent>()->isStatic = true;

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

    GameObject* obj = scena1->CreateGameObject(nullptr);
    CameraComponent* camCompLeft = obj->AddComponent<CameraComponent>();

    GameObject* obj2 = scena1->CreateGameObject(nullptr);
    CameraComponent* camCompRight = obj2->AddComponent<CameraComponent>();

    obj->GetComponent<TransformComponent>()->position = glm::vec3(0.0f, 20.0f, 50.0f);
     CameraHelper::InitialCamera(*camCompLeft,
         glm::vec3(0.0f, 1.0f, 0.0f),
         YAW,
         PITCH,
         Viewport{ 0.0f, 0.0f, 0.5f, 1.0f }
     );

    camCompLeft->isActive = true;

    obj2->GetComponent<TransformComponent>()->position = glm::vec3(50.0f, 30.0f, 0.0f);
    CameraHelper::InitialCamera(*camCompRight,
        glm::vec3(0.0f, 1.0f, 0.0f),
        0.0f, -20.0f,
        Viewport{ 0.5f, 0.0f, 0.5f, 1.0f }
    );

    camCompRight->isActive = true;

    GameObject* obj_Sprite_1 = scena1->CreateGameObject(nullptr);
    SpriteComponent* sprite_1 = obj_Sprite_1->AddComponent<SpriteComponent>();

    sprite_1->sprites.push_back(
        ResourceManager::LoadTexture("sigma.png", "res/textures/PGK_placeholders")
    );
    sprite_1->screenPosition = glm::vec2(0.0f, 0.0f);
    sprite_1->size = glm::vec2(128.0f, 128.0f);
    sprite_1->frameDuration = 0.15f;

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

    GameObject* obj_Sprite_4 = scena1->CreateGameObject(nullptr);
    SpriteComponent* sprite_4 = obj_Sprite_4->AddComponent<SpriteComponent>();
    sprite_4->textEnabled = true;
    sprite_4->screenPosition = glm::vec2(0.0f, 256.0f);

    sprite_1->layer = 1;
    sprite_2->layer = 1;
    sprite_4->layer = 1;

    int placeholderThing = 0;

    bed1Model = std::make_unique<Prefab>("res/models/samochod.glb");
    bed2Model      = std::make_unique<Prefab>("res/models/bed2.glb");
    bed3Model      = std::make_unique<Prefab>("res/models/bed3.glb");
    corkBoardModel = std::make_unique<Prefab>("res/models/cork_board.glb");
    cupModel       = std::make_unique<Prefab>("res/models/cup.glb");
    deskModel      = std::make_unique<Prefab>("res/models/desk.glb");
    doorsModel     = std::make_unique<Prefab>("res/models/doors.glb");
    folderModel    = std::make_unique<Prefab>("res/models/folder.glb");
    krzesloModel   = std::make_unique<Prefab>("res/models/krzeslo.glb");
    //ksiazkaModel   = std::make_unique<Prefab>("res/models/ksiazka.glb");
    lampa1Model    = std::make_unique<Prefab>("res/models/lampa1.glb");
    lampa2Model    = std::make_unique<Prefab>("res/models/lampa2.glb");
    lampa3Model    = std::make_unique<Prefab>("res/models/lampa3.glb");
    needleModel    = std::make_unique<Prefab>("res/models/needle.glb");
    bad1Model      = std::make_unique<Prefab>("res/models/obiekty_bad1.glb");
    bad2Model      = std::make_unique<Prefab>("res/models/obiekty_bad2.glb");
    bad3Model      = std::make_unique<Prefab>("res/models/obiekty_bad3.glb");
    papersModel    = std::make_unique<Prefab>("res/models/papers.glb");
    bossModel      = std::make_unique<Prefab>("res/models/placeholder_boss.glb");
    characterModel = std::make_unique<Prefab>("res/models/placeholder_character.glb");
    vial1Model     = std::make_unique<Prefab>("res/models/probowka1.glb");
    vial2Model     = std::make_unique<Prefab>("res/models/probowka2.glb");
    vial3Model     = std::make_unique<Prefab>("res/models/probowka3.glb");
    vial4Model     = std::make_unique<Prefab>("res/models/probowka4.glb");
    vial5Model     = std::make_unique<Prefab>("res/models/probowka5.glb");
    vial61Model    = std::make_unique<Prefab>("res/models/probowka6.glb");
    vial7Model     = std::make_unique<Prefab>("res/models/probowka7.glb");
    sinkModel      = std::make_unique<Prefab>("res/models/sink.glb");
    szafa1Model    = std::make_unique<Prefab>("res/models/szafa1.glb");
    szafa2Model    = std::make_unique<Prefab>("res/models/szafa2.glb");
    szafa3Model    = std::make_unique<Prefab>("res/models/szafa3.glb");
    telephoneModel = std::make_unique<Prefab>("res/models/telephone.glb");
    toiletModel    = std::make_unique<Prefab>("res/models/toilet.glb");
    wozekModel     = std::make_unique<Prefab>("res/models/wozek.glb");
    zaslonaModel   = std::make_unique<Prefab>("res/models/zaslona.glb");
    roomModel = std::make_unique<Prefab>("res/models/room.glb");


    //GameObject* model1 = bed1Model->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model2 = bed2Model->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model3 = bed3Model->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model4 = corkBoardModel->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model5 = cupModel      ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model6 = deskModel     ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model7 = doorsModel    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model8 = folderModel   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model9 = krzesloModel  ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model10 =ksiazkaModel  ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model11 =lampa1Model   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model12 =lampa2Model   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model13 =lampa3Model   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model14 =needleModel   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model15 =bad1Model     ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model16 =bad2Model     ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model17 =bad3Model     ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model18 =papersModel   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model19 =bossModel     ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model20 =characterModel->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model21 =vial1Model    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model22 =vial2Model    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model23 =vial3Model    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model24 =vial4Model    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model25 =vial5Model    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model26 =vial61Model   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model27 =vial7Model    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model28 =sinkModel     ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model29 =szafa1Model   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model30 =szafa2Model   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model31 =szafa3Model   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model32 =telephoneModel->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model33 =toiletModel   ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model34 =wozekModel    ->Instantiate(*scena1, nullptr, ourShader.get());
    //GameObject* model35 =zaslonaModel  ->Instantiate(*scena1, nullptr, ourShader.get());

    GameObject* roomObj = roomModel->Instantiate(*scena1, nullptr, ourShader.get());

    //RenderHelper::SetMaterial(model29, brickMat);

    //model1  ->GetComponent<TransformComponent>()->position.x = placeholderThing;

    //model1->AddComponent<RigidbodyComponent>();
    //model1->AddComponent<ColliderComponent>();
    
    //model1->GetComponent<RigidbodyComponent>()->useGravity = true;
    //model1->GetComponent<ColliderComponent>()->halfSize = glm::vec3{ 1, 1, 1 };
    //model1->GetComponent<TransformComponent>()->position.y = 150;

    //placeholderThing += 10;
    //model2  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model3  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model4  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model5  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model6  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model7  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model8  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model9  ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model10 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model11 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model12 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model13 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model14 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model15 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model16 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model17 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model18 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model19 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model20 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model21 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model22 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model23 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model24 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model25 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model26 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model27 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model28 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model29 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model30 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model31 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model32 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model33 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model34 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;
    //model35 ->GetComponent<TransformComponent>()->position.x = placeholderThing;
    //placeholderThing += 10;

    //CameraComponent* cam = obj2->AddComponent<CameraComponent>();
    //cam->camera.Position = glm::vec3(0, 50, 15);
    //cam->camera.Yaw = -90.0f;
    //cam->camera.Pitch = -10.0f;
    //cam->camera.updateCameraVectors();

    sceneManager.Update(16);

    spdlog::info("Scena git.");
    //compileShader();

    //createHouse();
    //startGroupInstanced(root.get());


    focused = true;
    updateFocus();


    int test_score = 0;
    auto* t0 = obj->GetComponent<TransformComponent>();
    auto* t1 = obj2->GetComponent<TransformComponent>();

    renderSystem = ecs.GetSystem<RenderSystem>();
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        updateFPS(deltaTime);


        processCameraInput(ecs, *camCompLeft, *t0,
    "move_up", "move_down", "move_left", "move_right");

        processCameraInput(ecs, *camCompRight, *t1,
            "move_up_2", "move_down_2", "move_left_2", "move_right_2");

        processCameraMouse(ecs, *camCompLeft);
        processCameraGamepad(ecs, *camCompLeft, *t0, 0);
        processCameraGamepad(ecs, *camCompRight, *t1, 1);
        //spdlog::info("GameObject position: x={}, y={}, z={}",
        //    transform->position.x,
        //    transform->position.y,
        //    transform->position.z);
        test_score++;
        sprite_4->text = "score: " + std::to_string(test_score);

        CpuTimer cpuTimer;
        cpuTimer.start();

        // --- CPU WORK START ---

        auto inputStart = std::chrono::high_resolution_clock::now();

        if (ecs.GetSystem<HID>()->is_action_just_pressed("right_click")) {
            focused = !focused;
            updateFocus();
        }

        // Process I/O operations here
        input();

        processCameraInput(ecs, *camCompLeft, *t0,
    "move_up", "move_down", "move_left", "move_right");

        processCameraInput(ecs, *camCompRight, *t1,
            "move_up_2", "move_down_2", "move_left_2", "move_right_2");

        processCameraMouse(ecs, *camCompLeft);
        processCameraGamepad(ecs, *camCompLeft, *t0, 0);
        processCameraGamepad(ecs, *camCompRight, *t1, 1);

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
        imgui_render(); // edit this function to add your own ImGui controls
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
/*
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
        if (!mouseMove) {
            firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        mouseMove = true;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE) {
        if (mouseMove) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        mouseMove = false;
    }
    */

//OLD INPUT ENDS HERE
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

void imgui_render()
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
    ImGui::PlotLines("Frame time", frameTimes, MAX_SAMPLES, index,
                 nullptr, 0.0f, 1.0f, ImVec2(0, 60));

    ImGui::End();

    ImGui::SliderFloat("rotation X", &rotationX, -480.0f, 480.0f);
    ImGui::SliderFloat("rotation Y", &rotationY, -480.0f, 480.0f);
    ImGui::SliderFloat("Camera Distance", &cameraDistance, 5.0f, 1000.0f);

    ImGui::Text("This is some useful text.");
    ImGui::Checkbox("Demo Window", &show_demo_window);
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::ColorEdit3("clear color", (float*)&clear_color);

    ImGui::End(); // 🔥 MUSI BYĆ

    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
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

/*
// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


*/
void processCameraGamepad(ECS &ecs, CameraComponent &cam, TransformComponent &transform, int gamepad_id) {
    const auto& hid = ecs.GetSystem<HID>();

    // ruch
    float lx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_X, gamepad_id);
    float ly = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_Y, gamepad_id);

    glm::vec3 dir(0.0f);

    dir += cam.state.Front * (-ly); // przód/tył
    dir += cam.state.Right * lx;    // lewo/prawo

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        transform.position += dir * MovementSpeed * deltaTime;
    }

    // obrót kamery
    float rx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_X, gamepad_id);
    float ry = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_Y, gamepad_id);

    const float sensitivity = 100.0f;

    CameraHelper::ProcessMouseMovement(cam,
        rx * sensitivity * deltaTime,
        -ry * sensitivity * deltaTime
    );
}


//
//void render()
//{
////OLD RENDER FUNCION STARTS HERE
//    
//    //float time = glfwGetTime();
//    //float radius = 10.0f;         
//    //float speed = 2.0f;        
//
//
//    //pointLight->position.x = radius * cos(speed * time);
//    //pointLight->position.y = 1.0f; 
//    //pointLight->position.z = radius * sin(speed * time);
//
//    if (!mouseMove)
//    {
//        updateFollowCamera();
//    }
//
//    // OpenGL Rendering code goes here
//    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    //bind texture
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, texture);
//
//    //activating program object
//    ourShader->use();
//   
//
//    int display_w, display_h;
//    glfwGetFramebufferSize(window, &display_w, &display_h);
//    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)display_w / (float)display_h, 0.1f, 10000.0f);
//    glm::mat4 view = glm::mat4(1.0f);
//    view = camera.GetViewMatrix();
//
//
//    // render the loaded modeld
//    root->updateSelfAndChild();
//    glm::mat4 systemRotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotationX), glm::vec3(1, 0, 0));
//    glm::mat4 systemRotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotationY), glm::vec3(0, 1, 0));
//    glm::mat4 systemModel = systemRotationY * systemRotationX;
//    renderGroup(projection, view, systemModel);
//    //ourEntity->drawSelfAndChild(*ourShader, projection, view, sphereRadius, sphereRings, sphereSectors, systemModel);
//
//
//    //glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);
//
//    // draw skybox as last
//    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
////    skyboxShader->use();
//    view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
////    skyboxShader->setMat4("view", view);
////    skyboxShader->setMat4("projection", projection);
//    // skybox cube
////    glBindVertexArray(skyboxVAO);
////    glActiveTexture(GL_TEXTURE0);
////    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
////    glDrawArrays(GL_TRIANGLES, 0, 36);
////    glBindVertexArray(0);
/////    glDepthFunc(GL_LESS); // set depth function back to default*/
////OLD RENDER FUNCTION ENDS HERE
//
//    triangleShader->use();
//    glBindVertexArray(triangleVAO);
//    glDrawArrays(GL_TRIANGLES, 0, 3);
//    glBindVertexArray(0);
//
//}