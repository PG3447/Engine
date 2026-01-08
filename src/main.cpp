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
void renderInstanced(Model* model, Shader* shader, std::vector<Entity*>& entities);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void regenerateSphere();
void createHouse();

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

std::unique_ptr<Model> sunModel;

std::unique_ptr<Model> wallModel;
std::unique_ptr<Model> roofModel;



std::unique_ptr<Entity> root;
Entity* venus;

struct pair_hash {
    std::size_t operator()(const std::pair<Model*, Shader*>& p) const {
        return std::hash<Model*>()(p.first) ^ (std::hash<Shader*>()(p.second) << 1);
    }
};


std::unordered_map<std::pair<Model*, Shader*>, std::vector<Entity*>, pair_hash> instancedGroups;

void startGroupInstanced(Entity* root)
{
    instancedGroups.clear();
       std::vector<Entity*> stackEntity;
    stackEntity.push_back(root);

    while (!stackEntity.empty())
    {
        Entity* entity = stackEntity.back();
        stackEntity.pop_back();

        if (entity->pModel)
        {
            std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
            instancedGroups[key].push_back(entity);
        }

        for (auto& child : entity->children)
        {
            stackEntity.push_back(child.get());
        }
    }
;
}

void AddEntityToGroupInstanced(Entity* entity)
{
    if (!entity->pModel)
        return;
    
    std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
    std::vector<Entity*>& group = instancedGroups[key];
    
    group.push_back(entity);

 }

bool start = true;
bool change = false;

void renderGroup(glm::mat4 projection, glm::mat4 view, glm::mat4 systemModel)
{
    for (auto& [key, entities] : instancedGroups)
    {
        Model* model = key.first;
        Shader* shader = key.second;

        shader->use();
        shader->setMat4("projection", projection);
        shader->setMat4("view", view);

        if (entities.size() == 0)
        {
            continue;
        }
        else if (entities.size() == 1)
        {
            shader->setBool("useInstance", false);
            shader->setMat4("model", systemModel * entities[0]->transform.getModelMatrix());
            model->Draw(*shader);
        }
        else
        {
            shader->setBool("useInstance", true);
            renderInstanced(model, shader, entities);
            
        }
    }
    start = false;
    change = false;
}


void renderInstanced(Model* model, Shader* shader, std::vector<Entity*>& entities)
{
    if (start || change) {
        size_t numEntities = entities.size();
        glm::mat4* modelMatrices = new glm::mat4[numEntities];

        for (size_t i = 0; i < numEntities; ++i)
        {
            modelMatrices[i] = entities[i]->transform.getModelMatrix();
        }


        if (model->instanceVBO == 0)
            glGenBuffers(1, &model->instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, model->instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, numEntities * sizeof(glm::mat4), modelMatrices, GL_DYNAMIC_DRAW);
    }

    if (start) {
        for (unsigned int i = 0; i < model->meshes.size(); i++)
        {
            unsigned int VAO = model->meshes[i].VAO;
            glBindVertexArray(VAO);

            // matrix
            GLsizei vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
            glEnableVertexAttribArray(9);
            glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(10);
            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

            glVertexAttribDivisor(7, 1);
            glVertexAttribDivisor(8, 1);
            glVertexAttribDivisor(9, 1);
            glVertexAttribDivisor(10, 1);

            glBindVertexArray(0);
        }
    }

    model->Draw(*shader, (GLsizei)entities.size());
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

    //loadTexture();
    compileShader();
    createHouse();
    startGroupInstanced(root.get());
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

    sunModel = std::make_unique<Model>("res/backpack/Sun.glb", 2.0f);

    wallModel = std::make_unique<Model>("res/backpack/sciany.glb");
    roofModel = std::make_unique<Model>("res/backpack/dach.glb");

    root = std::make_unique<Entity>();
    root->name = "root";



    spdlog::info("Success");

}


void createHouse()
{
    Entity* houses = root->addChild();
    houses->name = "domki";


    int GRID_X = 50;
    int GRID_Z = 50;
    float SPACING = 10.0f;

    for (int x = 0; x < GRID_X; ++x)
    {
        for (int z = 0; z < GRID_Z; ++z)
        {
            Entity* house = houses->addChild();
            house->name = "domek" + std::to_string(x) + std::to_string(z);


            Entity* wall = house->addChild(wallModel.get(), ourShader.get());
            wall->name = "sciany";
            Entity* roof = house->addChild(roofModel.get(), ourShader.get());
            roof->name = "dach";

            glm::vec3 pos;
            pos.x = x * SPACING;
            pos.y = 1.0f;
            pos.z = -z * SPACING;

            house->transform.setLocalPosition(pos);

            pos.x = 0.0f;
            pos.y = 2.0f;
            pos.z = 0.0f;

            roof->transform.setLocalPosition(pos);

            // losowa rotacja / skala
            // house->transform.setLocalRotation(glm::vec3(0.0f, rand() % 360, 0.0f));
            // house->transform.setLocalScale(glm::vec3(0.8f));
        }
    }
    // Wstępne obliczenie pozycji orbit
    root->updateSelfAndChild();
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
        //mouseMove = false;
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
    root->updateOrbit(deltaTime); 
    root->updateSelfAndChild();
    glm::mat4 systemRotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotationX), glm::vec3(1, 0, 0));
    glm::mat4 systemRotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotationY), glm::vec3(0, 1, 0));
    glm::mat4 systemModel = systemRotationY * systemRotationX;
    renderGroup(projection, view, systemModel);
    //ourEntity->drawSelfAndChild(*ourShader, projection, view, sphereRadius, sphereRings, sphereSectors, systemModel);


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

void showEntityTree(Entity* entity)
{
    // Używamy ImGui::TreeNode do wyświetlania hierarchii
    if (ImGui::TreeNode((void*)entity, "%s", entity->name.c_str()))
    {
        // Wyświetlamy każde dziecko rekurencyjnie
        for (auto& child : entity->children)
        {
            showEntityTree(child.get());
        }
        ImGui::TreePop();
    }
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
        
        if (root) // główny obiekt sceny
        {
            showEntityTree(root.get());
        }
        
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

