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
#include <filesystem> 



static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool init();
void init_imgui();


void compileShader();
void loadTexture();

void input();
void update();
void render();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void regenerateSphere();
void createSolarSystem();

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
Camera camera(glm::vec3(0.0f, 0.0f, 50.0f));
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseMove = false;

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
ImVec4 fractalColor = ImVec4(0.9f, 0.85f, 0.8f, 1.00f);
bool   autoRotation = false;

GLuint VBO;
GLuint VAO;
GLuint texture;
std::unique_ptr<Shader> ourShader;
std::unique_ptr<Shader> sphereShader;

std::unique_ptr<Model> sphereVenusModel;

std::unique_ptr<Model> ourModel;
std::unique_ptr<Model> emptyModel;

std::unique_ptr<Model> sunModel;
std::unique_ptr<Model> mercuryModel;
std::unique_ptr<Model> venusModel;
std::unique_ptr<Model> earthModel;
std::unique_ptr<Model> marsModel;
std::unique_ptr<Model> jupiterModel;
std::unique_ptr<Model> europaModel;
std::unique_ptr<Model> saturnModel;
std::unique_ptr<Model> mimasModel;
std::unique_ptr<Model> uranusModel;
std::unique_ptr<Model> mirandaModel;
std::unique_ptr<Model> neptuneModel;
std::unique_ptr<Model> tritonModel;

std::unique_ptr<Model> moonModel;
std::unique_ptr<Model> phobosModel;
std::unique_ptr<Model> deimosModel;


std::unique_ptr<Entity> ourEntity;
Entity* venus;

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

    //loadTexture();
    compileShader();
    createSolarSystem();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process I/O operations here
        input();

        // Update game objects' state here
        update();

        // OpenGL rendering code here
        render();

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
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Dear ImGui GLFW+OpenGL4 example", NULL, NULL);
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
    glEnable(GL_DEPTH_TEST);

    sphereShader = std::make_unique<Shader>("res/shaders/sphere.vert", "res/shaders/sphere.frag", "res/shaders/sphere.geom");
    
    sphereShader->use();

    ourShader = std::make_unique<Shader>("res/shaders/basic.vert", "res/shaders/basic.frag");

    ourShader->use();
    std::cout << "Current path: " << std::filesystem::current_path() << std::endl;


    sphereVenusModel = Model::createSphere(sphereRings, sphereSectors, "res/backpack/venusSurface.jpg");

    ourModel = std::make_unique<Model>("res/backpack/backpack.obj");

    sunModel = std::make_unique<Model>("res/backpack/Sun.glb", 2.0f);
    
    mercuryModel = std::make_unique<Model>("res/backpack/Mercury.glb", 0.002f);
    venusModel = std::make_unique<Model>("res/backpack/Venus.glb", 0.002f);
    earthModel = std::make_unique<Model>("res/backpack/Earth.glb", 0.002f);
    marsModel = std::make_unique<Model>("res/backpack/Mars.glb", 0.002f);
    jupiterModel = std::make_unique<Model>("res/backpack/Jupiter.glb", 0.002f);
    europaModel = std::make_unique<Model>("res/backpack/Europa.glb", 0.001f);
    saturnModel = std::make_unique<Model>("res/backpack/Saturn.glb", 0.002f);
    mimasModel = std::make_unique<Model>("res/backpack/Mimas.glb", 0.002f);
    uranusModel = std::make_unique<Model>("res/backpack/Uranus.glb", 0.002f);
    mirandaModel = std::make_unique<Model>("res/backpack/Miranda.glb", 0.002f);
    neptuneModel = std::make_unique<Model>("res/backpack/Neptune.glb", 0.002f);
    tritonModel = std::make_unique<Model>("res/backpack/Triton.glb", 0.002f);


    moonModel = std::make_unique<Model>("res/backpack/Moon.glb", 0.001f);
    phobosModel = std::make_unique<Model>("res/backpack/Phobos.glb", 0.02f);
    deimosModel = std::make_unique<Model>("res/backpack/Deimos.glb", 0.02f);

    emptyModel = std::make_unique<Model>("");
    
    ourEntity = std::make_unique<Entity>(*emptyModel);

    spdlog::info("Success");

}

std::unique_ptr<Model> modelOribtMercury;
std::unique_ptr<Model> modelOribtVenus;
std::unique_ptr<Model> modelOribtEarth;
std::unique_ptr<Model> modelOribtMoon;
std::unique_ptr<Model> modelOribtMars;
std::unique_ptr<Model> modelOribtPhobos;
std::unique_ptr<Model> modelOribtDeimos;
std::unique_ptr<Model> modelOribtJupiter;
std::unique_ptr<Model> modelOrbitMoonsJupiter[4];
std::unique_ptr<Model> modelOribtSaturn;
std::unique_ptr<Model> modelOribtUranus;
std::unique_ptr<Model> modelOribtMirandas;
std::unique_ptr<Model> modelOrbitMoonsSaturn[2];
std::unique_ptr<Model> modelOribtNeptun;
std::unique_ptr<Model> modelOribtTritons;

void createSolarSystem()
{
    const float scaleFactor = 1.5f;  // zwiększamy wszystkie odległości i promienie 10x
    const float moonScale = 1.5f;     // księżyce w mniejszej skali

    ourEntity->addChild(*emptyModel);
    Entity* sunOrbit = ourEntity->children.back().get();
 

    // Słońce
    ourEntity->addChild(*sunModel);
    Entity* sun = ourEntity->children.back().get();
    sun->initialOrbit(0, 0, 0, 0.0f);

    // Merkury
    sunOrbit->addChild(*emptyModel);
    Entity* mercuryOrbit = sunOrbit->children.back().get();
    mercuryOrbit->initialOrbit(40, 100, 6 * scaleFactor, 7.0f);
    mercuryOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -0.03f));

    mercuryOrbit->addChild(*mercuryModel);
    Entity* mercury = mercuryOrbit->children.back().get();
   
    // Wenus
    sunOrbit->addChild(*emptyModel);
    Entity* venusOrbit = sunOrbit->children.back().get();
    venusOrbit->initialOrbit(4, 50, 11 * scaleFactor, 3.4f);
    venusOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -177.4f));
    
    venusOrbit->addChild(*sphereVenusModel);
    venus = venusOrbit->children.back().get();
    venus->pCustomShader = sphereShader.get();

    // Ziemia
    sunOrbit->addChild(*emptyModel);
    Entity* earthOrbit = sunOrbit->children.back().get();
    earthOrbit->initialOrbit(5, 30, 15 * scaleFactor, 7.25f);
    earthOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -23.5f));


    earthOrbit->addChild(*earthModel);
    Entity* earth = earthOrbit->children.back().get();

    // Księżyc
    earthOrbit->addChild(*emptyModel);
    Entity* moonOrbit = earthOrbit->children.back().get();
    moonOrbit->initialOrbit(5, 150, 2 * moonScale, 5.1f);
    moonOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -6.68f));

    moonOrbit->addChild(*moonModel);
    Entity* moon = moonOrbit->children.back().get();
  

    // Mars
    sunOrbit->addChild(*emptyModel);
    Entity* marsOrbit = sunOrbit->children.back().get();
    marsOrbit->initialOrbit(8, 24, 22 * scaleFactor, 1.85f);
    marsOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -25.0f));

    marsOrbit->addChild(*marsModel);
    Entity* mars = marsOrbit->children.back().get();
    

     // Phobos
     marsOrbit->addChild(*emptyModel);
     Entity* phobosOrbit = marsOrbit->children.back().get();
     phobosOrbit->initialOrbit(0, 200, 1 * moonScale, 1.1f);
     phobosOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -1.0f));
    
     phobosOrbit->addChild(*phobosModel);
     Entity* phobos = phobosOrbit->children.back().get();
    
    
     // Deimos
     marsOrbit->addChild(*emptyModel);
     Entity* deimosOrbit = marsOrbit->children.back().get();
     deimosOrbit->initialOrbit(180, 100, 1.5f * moonScale, 1.8f);
     deimosOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -1.0f));
    
     deimosOrbit->addChild(*deimosModel);
     Entity* deimos = deimosOrbit->children.back().get();
     
     // Jowisz
     sunOrbit->addChild(*emptyModel);
     Entity* jupiterOrbit = sunOrbit->children.back().get();
     jupiterOrbit->initialOrbit(0, 12, 50 * scaleFactor, 1.3f);
     jupiterOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -3.0f));

     jupiterOrbit->addChild(*jupiterModel);
     Entity* jupiter = jupiterOrbit->children.back().get();
    
     // Księżyce Jowisza
     struct { float angle, speed, radius, tilt; } jupiterMoons[4] = {
         {0, 180, 2 * moonScale, 0.05f},
         {90, 140, 2.5f * moonScale, 0.1f},
         {180, 100, 3 * moonScale, 0.15f},
         {270, 80, 3.5f * moonScale, 0.2f}
     };
     for (auto& m : jupiterMoons)
     {
         jupiterOrbit->addChild(*emptyModel);
         Entity* moonJOrbit = jupiterOrbit->children.back().get();
         moonJOrbit->initialOrbit(m.angle, m.speed, m.radius, m.tilt);
         moonJOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, 0.0f));

         moonJOrbit->addChild(*europaModel);
         Entity* moonJ = moonJOrbit->children.back().get();
     }


     sunOrbit->addChild(*emptyModel);
     Entity* saturnOrbit = sunOrbit->children.back().get();
     saturnOrbit->initialOrbit(0, 10, 95 * scaleFactor, 2.49f);
     saturnOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -26.7f));
    
     // Saturn
     saturnOrbit->addChild(*saturnModel);
     Entity* saturn = saturnOrbit->children.back().get();
    
     // Księżyce Saturna
     struct { float angle, speed, radius, tilt; } saturnMoons[2] = {
         {0, 100, 4 * moonScale, 0.1f},
         {180, 120, 3 * moonScale, 0.2f}
     };
     for (auto& m : saturnMoons)
     {
         saturnOrbit->addChild(*emptyModel);
         Entity* moonSOrbit = saturnOrbit->children.back().get();
         moonSOrbit->initialOrbit(m.angle, m.speed, m.radius, m.tilt);
         moonSOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, 0.0f));
         
         moonSOrbit->addChild(*mimasModel);
         Entity* moonS = moonSOrbit->children.back().get();
     }
    
     // Uran
     sunOrbit->addChild(*emptyModel);
     Entity* uranusOrbit = sunOrbit->children.back().get();
     uranusOrbit->initialOrbit(0, 7, 145 * scaleFactor, 0.77f);
     uranusOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -97.8f));

     uranusOrbit->addChild(*uranusModel);
     Entity* uranus = uranusOrbit->children.back().get();

     // Księżyc Urana - dodajemy orbitę księżyca pod Uranem
     uranusOrbit->addChild(*emptyModel);
     Entity* mirandaOrbit = uranusOrbit->children.back().get();
     mirandaOrbit->initialOrbit(0, 90, 2 * moonScale, 0.2f);
     mirandaOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    
     mirandaOrbit->addChild(*mirandaModel);
     Entity* miranda = mirandaOrbit->children.back().get();

     // Neptun
     sunOrbit->addChild(*emptyModel);
     Entity* neptuneOrbit = sunOrbit->children.back().get();
     neptuneOrbit->initialOrbit(0, 5, 185 * scaleFactor, 1.77f);
     neptuneOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, -28.3f));

     neptuneOrbit->addChild(*neptuneModel);
     Entity* neptune = neptuneOrbit->children.back().get();
    
     // Księżyc Neptuna
     neptuneOrbit->addChild(*emptyModel);
     Entity* tritonOrbit = neptuneOrbit->children.back().get();
     tritonOrbit->initialOrbit(0, 80, 2 * moonScale, 0.2f);
     tritonOrbit->transform.setLocalRotation(glm::vec3(0.0f, 0.0f, 0.0f));

     tritonOrbit->addChild(*tritonModel);
     Entity* triton = tritonOrbit->children.back().get();

        // Slonce
    sun->setAppearance(glm::vec3(1.0f, 1.0f, 0.0f), glm::radians(425.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // Merkury
    mercury->setAppearance(glm::vec3(0.5f, 0.5f, 0.5f), glm::radians(184.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Wenus
    venus->setAppearance(glm::vec3(1.0f, 0.8f, 0.2f), glm::radians(-44.6f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Ziemia
    earth->setAppearance(glm::vec3(0.2f, 0.5f, 1.0f), glm::radians(10800.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Księżyc
    moon->setAppearance(glm::vec3(0.8f, 0.8f, 0.8f), glm::radians(395.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Mars
    mars->setAppearance(glm::vec3(1.0f, 0.3f, 0.2f), glm::radians(10515.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Phobos
    phobos->setAppearance(glm::vec3(0.6f, 0.6f, 0.6f), glm::radians(1129.1f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Deimos
    deimos->setAppearance(glm::vec3(0.7f, 0.7f, 0.7f), glm::radians(285.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Jowisz
    jupiter->setAppearance(glm::vec3(1.0f, 0.7f, 0.4f), glm::radians(26121.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Saturn
    saturn->setAppearance(glm::vec3(1.0f, 0.9f, 0.5f), glm::radians(24220.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Uran
    uranus->setAppearance(glm::vec3(0.5f, 0.9f, 1.0f), glm::radians(15050.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Neptun
    neptune->setAppearance(glm::vec3(0.3f, 0.5f, 1.0f), glm::radians(16117.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    modelOribtMercury = Model::createOrbit(6 * scaleFactor, 100, -7.0f, 1.0f, &mercury->pModel->textures_loaded);
    modelOribtVenus   = Model::createOrbit(11 * scaleFactor, 100, -3.4f, 1.0f, &venus->pModel->textures_loaded);
    modelOribtEarth   = Model::createOrbit(15.0f * scaleFactor, 100, -7.25f, 1.0f, &earth->pModel->textures_loaded);
    modelOribtMoon    = Model::createOrbit(2 * moonScale, 100, -5.1f, 1.0f, &moon->pModel->textures_loaded);
    modelOribtMars    = Model::createOrbit(22 * scaleFactor, 100, -1.85f, 1.0f, &mars->pModel->textures_loaded);
    modelOribtPhobos  = Model::createOrbit(1 * moonScale, 100, -1.1f, 1.0f, &phobos->pModel->textures_loaded);
    modelOribtDeimos  = Model::createOrbit(1.5f * moonScale, 100, -1.8f, 1.0f, &deimos->pModel->textures_loaded);
    modelOribtJupiter = Model::createOrbit(50 * scaleFactor,  100, -1.3f, 1.0f, &jupiter->pModel->textures_loaded);
    modelOribtSaturn = Model::createOrbit(95 * scaleFactor, 100, -2.49f, 1.0f, &saturn->pModel->textures_loaded);

    sunOrbit->addChild(*modelOribtMercury);
    sunOrbit->addChild(*modelOribtVenus);
    sunOrbit->addChild(*modelOribtEarth);
    earthOrbit->addChild(*modelOribtMoon);
    sunOrbit->addChild(*modelOribtMars);
    marsOrbit->addChild(*modelOribtPhobos);
    marsOrbit->addChild(*modelOribtDeimos);
    sunOrbit->addChild(*modelOribtJupiter);

    for (int i = 0; i < 4; ++i)
    {
        modelOrbitMoonsJupiter[i] = Model::createOrbit(jupiterMoons[i].radius, 100, -jupiterMoons[i].tilt, 1.0f, &moon->pModel->textures_loaded);
        jupiterOrbit->addChild(*modelOrbitMoonsJupiter[i]);
    }

    sunOrbit->addChild(*modelOribtSaturn);

    for (int i = 0; i < 4; ++i)
    {
        modelOrbitMoonsSaturn[i] = Model::createOrbit(saturnMoons[i].radius, 100, -saturnMoons[i].tilt, 1.0f, &moon->pModel->textures_loaded);
        saturnOrbit->addChild(*modelOrbitMoonsSaturn[i]);
    }

    modelOribtUranus = Model::createOrbit(145 * scaleFactor, 100, -0.77f, 1.0f, &uranus->pModel->textures_loaded);
    modelOribtMirandas = Model::createOrbit(2 * moonScale, 100, -0.2f, 1.0f, &miranda->pModel->textures_loaded);
    modelOribtNeptun = Model::createOrbit(185 * scaleFactor, 100, -1.77f, 1.0f, &neptune->pModel->textures_loaded);

    sunOrbit->addChild(*modelOribtUranus);
    uranusOrbit->addChild(*modelOribtMirandas);
    sunOrbit->addChild(*modelOribtNeptun);
    
    modelOribtTritons = Model::createOrbit(2 * moonScale, 100, -0.2f, 1.0f, &triton->pModel->textures_loaded);

    neptuneOrbit->addChild(*modelOribtTritons);


    // Wstępne obliczenie pozycji orbit
    ourEntity->updateOrbit(0.000001f);
    ourEntity->updateSelfAndChild();
}

void input()
{
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

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void update()
{
    // Update game objects' state here
}


void render()
{
    // OpenGL Rendering code goes here
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    //activating program object
    ourShader->use();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)display_w / (float)display_h, 0.1f, 10000.0f);
    glm::mat4 view = glm::mat4(1.0f);
    if (mouseMove)
    {
        view = camera.GetViewMatrix();
    }
    else
    {
        view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -cameraDistance));
    }

    // render the loaded model
    ourEntity->updateOrbit(deltaTime); 
    ourEntity->updateSelfAndChild();
    glm::mat4 systemRotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotationX), glm::vec3(1, 0, 0));
    glm::mat4 systemRotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotationY), glm::vec3(0, 1, 0));
    glm::mat4 systemModel = systemRotationY * systemRotationX;
    ourEntity->drawSelfAndChild(*ourShader, projection, view, sphereRadius, sphereRings, sphereSectors, systemModel);


    //glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
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

    if (mouseMove)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void regenerateSphere()
{
    if (!venus || !venus->pModel) return;
    Mesh& sphereMesh = venus->pModel->meshes[0];
    sphereMesh.updateSphereMesh(sphereRings, sphereSectors);
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
    /// Add new ImGui controls here
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)

        //ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.


        if (ImGui::Button(wireframeMode ? "Switch to Fill Mode" : "Switch to Wireframe"))
        {
            wireframeMode = !wireframeMode;
            if (wireframeMode)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // włącz wireframe
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // normalny render
        }

        if (ImGui::SliderInt("Rings", &sphereRings, 3, 25))
        {
            regenerateSphere();
        }
        
        // suwak do liczby sektorów
        if (ImGui::SliderInt("Sectors", &sphereSectors, 3, 25))
        {
            regenerateSphere();
        }
        //
        // suwak do promienia
        ImGui::SliderFloat("Radius", &sphereRadius, 0.1f, 10.0f);


        ImGui::SliderFloat("rotation X", &rotationX, -480.0f, 480.0f);
        ImGui::SliderFloat("rotation Y", &rotationY, -480.0f, 480.0f);
        ImGui::SliderFloat("Camera Distance", &cameraDistance, 5.0f, 1000.0f);


        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
        ImGui::ColorEdit3("fractal color", (float*)&fractalColor);

        if (ImGui::Button("Auto rotation"))
            autoRotation = !autoRotation;
        ImGui::SameLine();
        ImGui::Text(" = %s", autoRotation ? "true" : "false");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
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

