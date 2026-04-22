#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/ecs.h"

#include <model.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "skybox_renderer.h"
#include "../utils/camera_helper.h"



class RenderSystem : public System {
private:
    struct pair_hash {
        std::size_t operator()(const std::pair<Model*, Shader*>& p) const {
            return std::hash<Model*>()(p.first) ^ (std::hash<Shader*>()(p.second) << 1);
        }
    };

    Query<TransformComponent, RenderComponent>* renderQuery;
    Query<TransformComponent, CameraComponent>* cameraQuery;

    std::unordered_map<std::pair<Model*, Shader*>, std::vector<size_t>, pair_hash> instancedGroups;

    bool groupsDirty = true;

    GLFWwindow* window = nullptr;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    GLuint texture;

    SkyboxRenderer skybox;

    glm::mat4 projection;
    glm::mat4 view;

public:
    RenderSystem(ECS& ecs, GLFWwindow* win) : window(win) 
    {
        renderQuery = ecs.CreateQuery<TransformComponent, RenderComponent>();
        cameraQuery = ecs.CreateQuery<TransformComponent, CameraComponent>();
        
        Init();
    }

    void Init() {
        glEnable(GL_DEPTH_TEST);
        skybox.Init();
    }

    void OnGameObjectUpdated(GameObject* e) override {
        renderQuery->OnGameObjectUpdated(e); // forward do query
        cameraQuery->OnGameObjectUpdated(e); // forward do query

        groupsDirty = true;
    }

    void MarkDirty() {
        groupsDirty = true;
    }

    void Update(ECS& ecs) override {

        // OpenGL Rendering code goes here
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //bind texture
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, texture);

        BuildGroups();

        RenderAllCameras();

        //BuildGroups();
        //UpdateCamera();
        //RenderGroups();

        //glBindVertexArray(0);

        //skybox.Render(view, projection);
    }
    void ApplyViewport(const Viewport& vp, int w, int h)
    {
        glViewport(
            (GLint)(vp.x * w),
            (GLint)(vp.y * h),
            (GLsizei)(vp.width * w),
            (GLsizei)(vp.height * h)
        );
    }

    void RenderAllCameras() {
        auto& transforms = std::get<0>(cameraQuery->componentsVectors);
        auto& cameras = std::get<1>(cameraQuery->componentsVectors);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        for (size_t i = 0; i < cameras.size(); i++) {
            if (!cameras[i]->isActive)
                continue;

            RenderCamera(*cameras[i], *transforms[i], display_w, display_h);
        }
    }

    void RenderCamera(CameraComponent& cam, TransformComponent& transform, int width, int height) {
        
        ApplyViewport(cam.viewport, width, height);

        //view = cam.camera.beginRender(width, height);
        view = CameraHelper::getViewMatrix(cam, transform);
        projection = CameraHelper::getProjectionMatrix(cam);
        //projection = cam.camera.getProjectionMatrix(
        //    cam.nearPlane,
        //    cam.farPlane
        //);

        RenderGroups();

        glBindVertexArray(0);
        
        skybox.Render(view, projection);
    }

    void BuildGroups() {
        if (!groupsDirty) return;

        auto& renderers = std::get<1>(renderQuery->componentsVectors);

        instancedGroups.clear();

        for (size_t i = 0; i < renderQuery->gameobjects.size(); i++) {
            RenderComponent* r = renderers[i];

            if (!r || !r->model)
                continue;

            Shader* shader = r->shader;

            std::pair<Model*, Shader*> key = { r->model, shader };
            instancedGroups[key].push_back(i);
        }

        groupsDirty = false;
    }


    void RenderGroups() {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        auto& renderers = std::get<1>(renderQuery->componentsVectors);

        for (auto& [key, indices] : instancedGroups) {
            Model* model = key.first;
            Shader* shader = key.second;

            shader->use();
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);

            if (indices.size() == 1) {
                size_t i = indices[0];

                shader->setBool("useInstance", false);
                shader->setMat4("model", transforms[i]->modelMatrix);
                //spdlog::info("renderowanie");
                model->Draw(*shader);
            }
            else {
                shader->setBool("useInstance", true);
                RenderInstanced(model, shader, indices);
            }
        }
    }

    void RenderInstanced(Model* model, Shader* shader, std::vector<size_t>& indices)
    {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);

        size_t count = indices.size();
        std::vector<glm::mat4> matrices(count);

        for (size_t i = 0; i < count; i++) {
            matrices[i] = transforms[indices[i]]->modelMatrix;
        }
        
        if (model->instanceVBO == 0)
            glGenBuffers(1, &model->instanceVBO);

        model->PrepareInstancing();
        
        glBindBuffer(GL_ARRAY_BUFFER, model->instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::mat4), matrices.data(), GL_DYNAMIC_DRAW);

        model->Draw(*shader, (GLsizei)count);
    }
};

#endif