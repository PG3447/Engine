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
#include <camera.h>
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
void processCameraInput(ECS& ecs, Camera& cam,
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

/*
Camera camera(
    glm::vec3(0.0f, 20.0f, 50.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    YAW, PITCH,
    Viewport{ 0.0f, 0.0f, 0.5f, 1.0f }
);

Camera cameraRight(
    glm::vec3(50.0f, 30.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    0.0f, -20.0f,
    Viewport{ 0.5f, 0.0f, 0.5f, 1.0f }
);*/

// camera
Camera camera(glm::vec3(0.0f, 20.0f, 50.0f));
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

//
//struct pair_hash {
//    std::size_t operator()(const std::pair<Model*, Shader*>& p) const {
//        return std::hash<Model*>()(p.first) ^ (std::hash<Shader*>()(p.second) << 1);
//    }
//};
//
//
//std::unordered_map<std::pair<Model*, Shader*>, std::vector<Entity*>, pair_hash> instancedGroups;
//
//void startGroupInstanced(Entity* root)
//{
//    instancedGroups.clear();
//       std::vector<Entity*> stackEntity;
//    stackEntity.push_back(root);
//
//    while (!stackEntity.empty())
//    {
//        Entity* entity = stackEntity.back();
//        stackEntity.pop_back();
//
//        if (entity->pModel)
//        {
//            std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
//            instancedGroups[key].push_back(entity);
//        }
//
//        for (auto& child : entity->children)
//        {
//            stackEntity.push_back(child.get());
//        }
//    }
//;
//}
//
//void AddEntityToGroupInstanced(Entity* entity)
//{
//    if (!entity->pModel)
//        return;
//    
//    std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
//    std::vector<Entity*>& group = instancedGroups[key];
//    
//    group.push_back(entity);
//
// }
//
//bool start = true;
//bool change = false;
//
//void renderGroup(glm::mat4 projection, glm::mat4 view, glm::mat4 systemModel)
//{
//    for (auto& [key, entities] : instancedGroups)
//    {
//        Model* model = key.first;
//        Shader* shader = key.second;
//
//        shader->use();
//
//        shader->setVec3("viewPos", camera.Position);
//        shader->setVec3("cameraPos", camera.Position);
//
//        shader->setMat4("projection", projection);
//        shader->setMat4("view", view);
//
//        if (entities.size() == 0)
//        {
//            continue;
//        }
//        else if (entities.size() == 1)
//        {
//            shader->setBool("useInstance", false);
//            shader->setMat4("model", systemModel * entities[0]->transform.getModelMatrix());
//
//            if (entities[0]->pLight != nullptr)
//            {
//                entities[0]->transform.setLocalPosition(entities[0]->pLight->position);
//                glm::vec3 dir = glm::normalize(entities[0]->pLight->direction);
//
//                float yaw = atan2(dir.x, dir.z);
//                float pitch = asin(-dir.y);
//
//                entities[0]->transform.setLocalRotation(glm::degrees(glm::vec3(pitch, yaw, 0.0f)));
//                entities[0]->pLight->Apply(*shader);
//            }
//            
//            if (model != nullptr)
//            {
//                model->Draw(*shader);
//            }
//        }
//        else
//        {
//            shader->setBool("useInstance", true);
//            renderInstanced(model, shader, entities);
//            
//        }
//    }
//    start = false;
//    change = false;
//}
//
//
//void renderInstanced(Model* model, Shader* shader, std::vector<Entity*>& entities)
//{
//    if (start || change) {
//        size_t numEntities = entities.size();
//
//        std::vector<glm::mat4> modelMatrices(numEntities);
//        for (size_t i = 0; i < numEntities; ++i) {
//            modelMatrices[i] = entities[i]->transform.getModelMatrix();
//        }
//
//        if (model->instanceVBO == 0)
//            glGenBuffers(1, &model->instanceVBO);
//        glBindBuffer(GL_ARRAY_BUFFER, model->instanceVBO);
//        glBufferData(GL_ARRAY_BUFFER, numEntities * sizeof(glm::mat4), modelMatrices.data(), GL_DYNAMIC_DRAW);
//    }
//
//    if (start) {
//        for (unsigned int i = 0; i < model->meshes.size(); i++)
//        {
//            unsigned int VAO = model->meshes[i].renderData->VAO;
//            glBindVertexArray(VAO);
//
//            // matrix
//            GLsizei vec4Size = sizeof(glm::vec4);
//            glEnableVertexAttribArray(7);
//            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
//            glEnableVertexAttribArray(8);
//            glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
//            glEnableVertexAttribArray(9);
//            glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
//            glEnableVertexAttribArray(10);
//            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
//
//            glVertexAttribDivisor(7, 1);
//            glVertexAttribDivisor(8, 1);
//            glVertexAttribDivisor(9, 1);
//            glVertexAttribDivisor(10, 1);
//
//            glBindVertexArray(0);
//        }
//    }
//    if (model != nullptr)
//    {
//        model->Draw(*shader, (GLsizei)entities.size());
//    }
//}


void processCameraInput(ECS& ecs,Camera& cam,
                       const std::string& up,
                       const std::string& down,
                       const std::string& left,
                       const std::string& right)
{
    const auto& hid = ecs.GetSystem<HID>();
    glm::vec3 dir(0.0f);

    if (hid->is_action_pressed(up))    dir += cam.Front;
    if (hid->is_action_pressed(left))  dir -= cam.Right;
    if (hid->is_action_pressed(right)) dir += cam.Right;
    if (hid->is_action_pressed(down))  dir -= cam.Front;

    if (glm::length(dir) > 0.0f)
        dir = glm::normalize(dir);

    cam.Position += dir * cam.MovementSpeed * deltaTime;
}

void processCameraMouse(ECS& ecs, Camera& cam)
{
    const auto& hid = ecs.GetSystem<HID>();
    cam.ProcessMouseMovement(
        (float)hid->get_mouse_dx(),
        (float)-hid->get_mouse_dy()
    );
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


    ecs.AddSystem<TransformSystem>(ecs);
    ecs.AddSystem<PhysicsSystem>(ecs);
    ecs.AddSystem<RenderSystem>(ecs, window);
    ecs.AddSystem<HID>(ecs, window);
    ecs.AddSystem<SpriteSystem>(ecs, window);

    ourShader = std::make_unique<Shader>("res/shaders/basic.vert", "res/shaders/basic.frag");
    ourShader->use();

    groundModel = std::make_unique<Prefab>("res/backpack/podloze.glb");
    GameObject* obb = groundModel->Instantiate(*scena1, nullptr, ourShader.get());

    obb->GetComponent<TransformComponent>()->scale.x = 1000;
    obb->GetComponent<TransformComponent>()->scale.y = 1000;
    obb->GetComponent<TransformComponent>()->scale.z = 1000;

    GameObject* obj = scena1->CreateGameObject(nullptr);
    CameraComponent* camCompLeft = obj->AddComponent<CameraComponent>();

    GameObject* obj2 = scena1->CreateGameObject(nullptr);
    CameraComponent* camCompRight = obj2->AddComponent<CameraComponent>();

    camCompLeft->camera = Camera(
        glm::vec3(0.0f, 20.0f, 50.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        YAW, PITCH,
        Viewport{ 0.0f, 0.0f, 0.5f, 1.0f }
    );

    camCompLeft->isActive = true;

    camCompRight->camera = Camera(
        glm::vec3(50.0f, 30.0f, 0.0f),
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
    ecs.GetSystem<HID>()->name_action("move_right", GLFW_KEY_D);
    ecs.GetSystem<HID>()->name_action("move_left", GLFW_KEY_A);
    ecs.GetSystem<HID>()->name_action("move_up", GLFW_KEY_W);
    ecs.GetSystem<HID>()->name_action("move_down", GLFW_KEY_S);
    ecs.GetSystem<HID>()->name_action("move_right1", GLFW_KEY_RIGHT);
    ecs.GetSystem<HID>()->name_action("move_left1", GLFW_KEY_LEFT);
    ecs.GetSystem<HID>()->name_action("move_up1", GLFW_KEY_UP);
    ecs.GetSystem<HID>()->name_action("move_down1", GLFW_KEY_DOWN);
    ecs.GetSystem<HID>()->name_action_mouse("move_right", GLFW_MOUSE_BUTTON_LEFT);
    ecs.GetSystem<HID>()->name_action_gamepad("move_right", GLFW_GAMEPAD_BUTTON_SQUARE, 0);
    ecs.GetSystem<HID>()->name_action_gamepad("move_right_1", GLFW_GAMEPAD_BUTTON_SQUARE, 1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int test_score = 0;
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processCameraInput(ecs , camCompLeft->camera, "move_up", "move_down", "move_left", "move_right");
        processCameraMouse(ecs, camCompLeft->camera);
        //spdlog::info("GameObject position: x={}, y={}, z={}",
        //    transform->position.x,
        //    transform->position.y,
        //    transform->position.z);
        test_score++;
        sprite_4->text = "score: " + std::to_string(test_score);
        sceneManager.Update(16);

        // Process I/O operations here
        input();

        // Update game objects' state here
        update();

        // OpenGL rendering code here
        //render();

        // Draw ImGui
        imgui_begin();
        imgui_render(); // edit this function to add your own ImGui controls
        imgui_end(); // this call effectively renders ImGui

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
    //glEnable(GL_DEPTH_TEST);


    //float skyboxVertices[] = {
    //    // positions          
    //    -1.0f,  1.0f, -1.0f,
    //    -1.0f, -1.0f, -1.0f,
    //     1.0f, -1.0f, -1.0f,
    //     1.0f, -1.0f, -1.0f,
    //     1.0f,  1.0f, -1.0f,
    //    -1.0f,  1.0f, -1.0f,

    //    -1.0f, -1.0f,  1.0f,
    //    -1.0f, -1.0f, -1.0f,
    //    -1.0f,  1.0f, -1.0f,
    //    -1.0f,  1.0f, -1.0f,
    //    -1.0f,  1.0f,  1.0f,
    //    -1.0f, -1.0f,  1.0f,

    //     1.0f, -1.0f, -1.0f,
    //     1.0f, -1.0f,  1.0f,
    //     1.0f,  1.0f,  1.0f,
    //     1.0f,  1.0f,  1.0f,
    //     1.0f,  1.0f, -1.0f,
    //     1.0f, -1.0f, -1.0f,

    //    -1.0f, -1.0f,  1.0f,
    //    -1.0f,  1.0f,  1.0f,
    //     1.0f,  1.0f,  1.0f,
    //     1.0f,  1.0f,  1.0f,
    //     1.0f, -1.0f,  1.0f,
    //    -1.0f, -1.0f,  1.0f,

    //    -1.0f,  1.0f, -1.0f,
    //     1.0f,  1.0f, -1.0f,
    //     1.0f,  1.0f,  1.0f,
    //     1.0f,  1.0f,  1.0f,
    //    -1.0f,  1.0f,  1.0f,
    //    -1.0f,  1.0f, -1.0f,

    //    -1.0f, -1.0f, -1.0f,
    //    -1.0f, -1.0f,  1.0f,
    //     1.0f, -1.0f, -1.0f,
    //     1.0f, -1.0f, -1.0f,
    //    -1.0f, -1.0f,  1.0f,
    //     1.0f, -1.0f,  1.0f
    //};

    //// skybox VAO
    //unsigned int skyboxVBO;
    //glGenVertexArrays(1, &skyboxVAO);
    //glGenBuffers(1, &skyboxVBO);
    //glBindVertexArray(skyboxVAO);
    //glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    //glEnableVertexAttribArray(0);
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    //vector<std::string> faces
    //{
    //    "res/textures/skybox/right.jpg",
    //    "res/textures/skybox/left.jpg",
    //    "res/textures/skybox/top.jpg",
    //    "res/textures/skybox/bottom.jpg",
    //    "res/textures/skybox/front.jpg",
    //    "res/textures/skybox/back.jpg"
    //};

    //cubemapTexture = loadCubemap(faces);

    //skyboxShader = std::make_unique<Shader>("res/shaders/skybox.vert", "res/shaders/skybox.frag");

//    skyboxShader->use();
//    skyboxShader->setInt("skybox", 0);

//    reflectShader = std::make_unique<Shader>("res/shaders/reflect.vert", "res/shaders/reflect.frag");
//    reflectShader->use();
//    reflectShader->setInt("skybox", 0);

//    refractShader = std::make_unique<Shader>("res/shaders/reflect.vert", "res/shaders/refract.frag");
//    refractShader->use();
//    refractShader->setInt("skybox", 0);

    //ladowanie bazowego shader-a
 
    sunModel = std::make_unique<Prefab>("res/backpack/sun.glb");
    
    root = std::make_unique<Entity>();
    root->name = "root";

   // float triangleVertices[] = {
   //     0.0f,  0.5f, 0.0f,
   //    -0.5f, -0.5f, 0.0f,
   //     0.5f, -0.5f, 0.0f
   //};

   // glGenVertexArrays(1, &triangleVAO);
   // glGenBuffers(1, &triangleVBO);

   // glBindVertexArray(triangleVAO);

   // glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
   // glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

   // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
   // glEnableVertexAttribArray(0);

   // glBindVertexArray(0);

   // // shader
   // triangleShader = std::make_unique<Shader>(
   //     "res/shaders/triangle.vert",
   //     "res/shaders/triangle.frag"
   // );


    spdlog::info("Success");

}
//
//std::unique_ptr<Light> dircetLight;
////std::unique_ptr<Light> pointLight;
////std::unique_ptr<Light> spotLight;
////std::unique_ptr<Light> spotLight2;
//
//std::unique_ptr<Model> emptyModel;
//std::unique_ptr<Prefab> emptyModel1;
//std::unique_ptr<Prefab> emptyModel2;
//std::unique_ptr<Prefab> emptyModel3;
//
//
//void createHouse()
//{
//    Entity* ground = groundModel->getEntitiesCreate(ourShader.get());
//    ground->name = "podloze";
//    ground->transform.setLocalScale(glm::vec3(100.0f));
//    root->addChild(ground);
//    
//    emptyModel = std::make_unique<Model>();
//    //emptyModel1 = std::make_unique<Prefab>("res/backpack/podloze.glb", 0.25f, false);
//    //emptyModel2 = std::make_unique<Prefab>("res/backpack/podloze.glb", 1.0f, false);
//    //emptyModel3 = std::make_unique<Prefab>("res/backpack/podloze.glb", 1.0f, false);
//    //
//    dircetLight = std::make_unique<Light>(Light::Directional);
//    dircetLight->direction = glm::vec3(-0.3f, -1.0f, -0.1f);
//    dircetLight->ambient  = glm::vec3(0.25f);
//    dircetLight->diffuse = glm::vec3(0.85f);
//    dircetLight->specular = glm::vec3(0.4f);
//    //
//    //pointLight = std::make_unique<Light>(Light::Point);
//    //pointLight->index = 0;
//    //pointLight->position = glm::vec3(-10.0f, 1.0f, -0.3f);
//    //pointLight->ambient = glm::vec3(0.05f);
//    //pointLight->diffuse = glm::vec3(0.8f);
//    //pointLight->specular = glm::vec3(1.0f);
//    //pointLight->constant = 1.0f;
//    //pointLight->linear = 0.09f;
//    //pointLight->quadratic = 0.032f;
//
//    //spotLight = std::make_unique<Light>(Light::Spot);
//    //spotLight->index = 0;
//    //spotLight->position = glm::vec3(-12.0f, 5.0f, 1.5f);
//    //spotLight->direction = glm::vec3(-0.15f, -0.9f, -0.25f);
//    //spotLight->ambient = glm::vec3(0.0f);
//    //spotLight->diffuse = glm::vec3(1.0f);
//    //spotLight->specular = glm::vec3(1.0f);
//    //spotLight->constant = 1.0f;
//    //spotLight->linear = 0.09f;
//    //spotLight->quadratic = 0.032f;
//    //spotLight->cutOff = glm::cos(glm::radians(12.5f));
//    //spotLight->outerCutOff = glm::cos(glm::radians(15.0f));
//
//
//    //spotLight2 = std::make_unique<Light>(Light::Spot);
//    //spotLight2->index = 1;
//    //spotLight2->position = glm::vec3(-2.0f, 5.0f, 1.5f);
//    //spotLight2->direction = glm::vec3(-0.15f, -0.9f, -0.25f);
//    //spotLight2->ambient = glm::vec3(0.0f);
//    //spotLight2->diffuse = glm::vec3(1.0f);
//    //spotLight2->specular = glm::vec3(1.0f);
//    //spotLight2->constant = 1.0f;
//    //spotLight2->linear = 0.09f;
//    //spotLight2->quadratic = 0.032f;
//    //spotLight2->cutOff = glm::cos(glm::radians(25.0f));
//    //spotLight2->outerCutOff = glm::cos(glm::radians(30.0f));
//
//    
//    Entity* directonalLight = root->addChild(emptyModel.get(), ourShader.get(), dircetLight.get());
//    directonalLight->name = "directonalLight";
//
//    //Entity* pointLightEnt = emptyModel1->getEntitiesCreate(ourShader.get(), pointLight.get());
//    //pointLightEnt->name = "pointLight";
//    //root->addChild(pointLightEnt);
//
//    //Entity* spotLightEnt = emptyModel2->getEntitiesCreate(ourShader.get(), spotLight.get());
//    //spotLightEnt->name = "spotLight";
//    //root->addChild(spotLightEnt);
//
//    //Entity* spotLight2Ent = emptyModel3->getEntitiesCreate(ourShader.get(), spotLight2.get());
//    //spotLight2Ent->name = "spotLight2";
//    //root->addChild(spotLight2Ent);
//    //
//    // Wstępne obliczenie pozycji orbit
//
//    Entity* sun = sunModel->getEntitiesCreate(ourShader.get());
//    sun->name = "sun";
//    sun->transform.setLocalPosition(glm::vec3(0.0f, 10.0f, 0.0f));
//    sun->transform.setLocalScale(glm::vec3(1.0f));
//    root->addChild(sun);
//    
//    root->updateSelfAndChild();
//}


//void updateFollowCamera()
//{
//    if (!koparkaEntity) return;
//
//    Transform& t = koparkaEntity->transform;
//
//    glm::vec3 koparkaPos = t.getGlobalPosition();
//    glm::vec3 back = -t.getForward();
//
//    glm::vec3 desiredPos =
//        koparkaPos +
//        back * cameraOffset.z +
//        glm::vec3(0.0f, cameraOffset.y, 0.0f);
//
//
//    camera.Position = glm::mix(camera.Position, desiredPos, 5.0f * deltaTime);
//
//    glm::vec3 lookTarget = koparkaPos + glm::vec3(0.0f, 5.0f, 0.0f);
//    camera.Front = glm::normalize(lookTarget - camera.Position);
//    camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0, 1, 0)));
//    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
//}


void input()
{
//OLD INPUT STARTS HERE

    // I/O ops go here
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

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
    //processCameraInput(camera, "move_up", "move_down", "move_left", "move_right");
    //processCameraInput(cameraRight, "move_up1", "move_down1", "move_left1", "move_right1");
    /*float velocity = camera.MovementSpeed * deltaTime;
        if (HID::get().is_action_pressed("move_up"))
            camera.Position += camera.Front * velocity;
        if (HID::get().is_action_pressed("move_down"))
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (HID::get().is_action_pressed("move_left"))
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (HID::get().is_action_pressed("move_right"))
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (HID::get().is_action_pressed("move_right1"))
            cameraRight.ProcessKeyboard(RIGHT, deltaTime);
        if (HID::get().is_action_pressed("move_left1"))
            cameraRight.ProcessKeyboard(LEFT, deltaTime);
        if (HID::get().is_action_pressed("move_up1"))
            cameraRight.ProcessKeyboard(FORWARD, deltaTime);
        if (HID::get().is_action_pressed("move_down1"))
            cameraRight.ProcessKeyboard(BACKWARD, deltaTime);*/
//OLD INPUT ENDS HERE
}


void update()
{
    // Update game objects' state here
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

    if (mouseMove)
        camera.ProcessMouseMovement(xoffset, yoffset);
}
//
//// glfw: whenever the mouse scroll wheel scrolls, this callback is called
//// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
//
//void regenerateSphere()
//{
//    if (!venus || !venus->pModel) return;
//    Mesh& sphereMesh = venus->pModel->meshes[0];
//    sphereMesh.updateSphereMesh(sphereRings, sphereSectors);
//}
//
void imgui_begin()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
//
////Wszystko ponizej to imgui
//static Entity* selectedEntity = nullptr;
//
//void showLightEditor(Light& light)
//{
//    ImGui::Checkbox("Light Enabled", &light.isOn);
//
//    const char* lightTypes[] = { "Directional", "Point", "Spot" };
//    int type = static_cast<int>(light.type);
//    if (ImGui::Combo("Light Type", &type, lightTypes, IM_ARRAYSIZE(lightTypes)))
//    {
//        light.type = static_cast<Light::LightType>(type);
//    }
//
//    ImGui::Separator();
//    ImGui::Text("Colors");
//
//    ImGui::ColorEdit3("Ambient", &light.ambient.x);
//    ImGui::ColorEdit3("Diffuse", &light.diffuse.x);
//    ImGui::ColorEdit3("Specular", &light.specular.x);
//
//    ImGui::Separator();
//
//    if (light.type != Light::Directional)
//    {
//        ImGui::DragFloat3("Position", &light.position.x, 0.05f);
//    }
//
//    if (light.type == Light::Directional || light.type == Light::Spot)
//    {
//        ImGui::DragFloat3("Direction", &light.direction.x, 0.05f);
//    }
//
//    if (light.type == Light::Point || light.type == Light::Spot)
//    {
//        ImGui::Separator();
//        ImGui::Text("Attenuation");
//
//        ImGui::DragFloat("Constant", &light.constant, 0.01f, 0.0f, 2.0f);
//        ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 1.0f);
//        ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 1.0f);
//    }
//
//    if (light.type == Light::Spot)
//    {
//        ImGui::Separator();
//        ImGui::Text("Spotlight Angles");
//
//        float inner = glm::degrees(acos(light.cutOff));
//        float outer = glm::degrees(acos(light.outerCutOff));
//
//        if (ImGui::SliderFloat("Inner CutOff", &inner, 0.0f, 90.0f))
//            light.cutOff = glm::cos(glm::radians(inner));
//
//        if (ImGui::SliderFloat("Outer CutOff", &outer, inner, 90.0f))
//            light.outerCutOff = glm::cos(glm::radians(outer));
//    }
//}
//
//
//void showTransformEditor(Transform& transform)
//{
//    glm::vec3 pos = transform.getLocalPosition();
//    glm::vec3 rot = transform.getLocalRotation();
//    glm::vec3 scale = transform.getLocalScale();
//
//    if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
//    {
//        change = true;
//        transform.setLocalPosition(pos);
//    }
//
//    if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
//    {
//        change = true;
//        transform.setLocalRotation(rot);
//    }
//
//    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.01f))
//    {
//        change = true;
//        transform.setLocalScale(scale);
//    }
//}
//static ImGuiTextFilter entityFilter;
//
//bool entityMatchesFilter(Entity* entity)
//{
//    if (entityFilter.PassFilter(entity->name.c_str()))
//        return true;
//
//    for (auto& child : entity->children)
//    {
//        if (entityMatchesFilter(child.get()))
//            return true;
//    }
//    return false;
//}
//
//
//void showEntityTree(Entity* entity)
//{
//    if (!entity) return;
//
//    if (!entityMatchesFilter(entity))
//        return;
//
//    ImGuiTreeNodeFlags flags =
//        ImGuiTreeNodeFlags_OpenOnArrow |
//        ImGuiTreeNodeFlags_OpenOnDoubleClick |
//        ((entity == selectedEntity) ? ImGuiTreeNodeFlags_Selected : 0);
//
//    bool nodeOpen = ImGui::TreeNodeEx(
//        (void*)entity,
//        flags,
//        "%s",
//        entity->name.c_str()
//    );
//
//    if (ImGui::IsItemClicked())
//        selectedEntity = entity;
//
//    if (nodeOpen)
//    {
//        for (auto& child : entity->children)
//            showEntityTree(child.get());
//
//        ImGui::TreePop();
//    }
//}
//

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

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
//
//void imgui_render()
//{
//    /// Add new ImGui controls here
//    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
//    if (show_demo_window)
//
//        //ImGui::ShowDemoWindow(&show_demo_window);
//
//    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
//
//    {
//        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
//
//
//        if (ImGui::Button(wireframeMode ? "Switch to Fill Mode" : "Switch to Wireframe"))
//        {
//            wireframeMode = !wireframeMode;
//            if (wireframeMode)
//                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // włącz wireframe
//            else
//                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // normalny render
//        }
//
//        ImGui::SliderFloat("rotation X", &rotationX, -480.0f, 480.0f);
//        ImGui::SliderFloat("rotation Y", &rotationY, -480.0f, 480.0f);
//        ImGui::SliderFloat("Camera Distance", &cameraDistance, 5.0f, 1000.0f);
//
//
//        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
//        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
//        ImGui::Checkbox("Another Window", &show_another_window);
//
//        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
//    }
//    //    if (ImGui::Button("Auto rotation"))
//    //        autoRotation = !autoRotation;
//    //    ImGui::SameLine();
//    //    ImGui::Text(" = %s", autoRotation ? "true" : "false");
//
//    //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//    //    
//    //   if (root) // główny obiekt sceny
//    //   {
//    //       ImGui::Separator();
//    //       ImGui::Text("Hierarchy");
//    //       entityFilter.Draw("Search", 200);
//    //       ImGui::Separator();
//
//    //       showEntityTree(root.get());
//
//    //   
//    //       if (selectedEntity)
//    //       {
//    //           ImGui::Separator();
//    //           ImGui::Text("Selected Entity: %s", selectedEntity->name.c_str());
//    //           showTransformEditor(selectedEntity->transform);
//    //       }
//
//
//    //       ImGui::Separator();
//    //       ImGui::Text("Lighting");
//
//    //       if (ImGui::CollapsingHeader("Directional Light"))
//    //       {
//    //           ImGui::PushID("Directional");
//    //           showLightEditor(*dircetLight);
//    //           ImGui::PopID();
//    //       }
//
//    //       //if (ImGui::CollapsingHeader("Point Lights"))
//    //       //{
//    //       //    ImGui::PushID("pointLight");
//    //       //    showLightEditor(*pointLight);
//    //       //    ImGui::PopID();
//    //       //}
//
//    //       //if (ImGui::CollapsingHeader("Spot Light"))
//    //       //{
//    //       //    ImGui::PushID("spotLight");
//    //       //    showLightEditor(*spotLight);
//    //       //    ImGui::PopID();
//    //       //}
//
//    //       //if (ImGui::CollapsingHeader("Spot Light2"))
//    //       //{
//    //       //    ImGui::PushID("spotLight2");
//    //       //    showLightEditor(*spotLight2);
//    //       //    ImGui::PopID();
//    //       //}
//    //   }
//    //   
//    //    
//    //    ImGui::End();
//    //}
//
//    // 3. Show another simple window.
//    if (show_another_window)
//    {
//        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
//        ImGui::Text("Hello from another window!");
//        if (ImGui::Button("Close Me"))
//            show_another_window = false;
//        ImGui::End();
//    }
//}

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



