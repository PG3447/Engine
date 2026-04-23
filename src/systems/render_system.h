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
    using GroupKey = std::tuple<Model*, Shader*, Material*>;

    struct group_hash {
        std::size_t operator()(const GroupKey& k) const {
            return std::hash<Model*>()(std::get<0>(k)) ^
                (std::hash<Shader*>()(std::get<1>(k)) << 1) ^
                (std::hash<Material*>()(std::get<2>(k)) << 2);
        }
    };

    Query<TransformComponent, RenderComponent>* renderQuery;
    Query<TransformComponent, CameraComponent>* cameraQuery;

    std::unordered_map<GroupKey, std::vector<size_t>, group_hash> instancedGroups;

    bool groupsDirty = true;

    GLFWwindow* window = nullptr;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    GLuint texture;

    SkyboxRenderer skybox;

    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 currentCameraPos;

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

        view = CameraHelper::getViewMatrix(cam, transform);
        projection = CameraHelper::getProjectionMatrix(cam, width, height);

        currentCameraPos = transform.position;

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
            if (!r || !r->model) continue;

            if (r->shader) {
                r->model->SetShader(r->shader);
            }

            GroupKey key = { r->model, r->shader, r->materialOverride.get() };
            instancedGroups[key].push_back(i);
        }
        groupsDirty = false;
    }

    void RenderGroups() {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        for (auto& [key, indices] : instancedGroups) {
            Model* model = std::get<0>(key);
            Shader* shader = std::get<1>(key);
            Material* overrideMat = std::get<2>(key);

            shader->use();
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);

            shader->setVec3("viewPos", currentCameraPos);
            shader->setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
            shader->setVec3("dirLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
            shader->setVec3("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
            shader->setVec3("dirLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

            if (indices.size() == 1) {
                shader->setBool("useInstance", false);
                shader->setMat4("model", transforms[indices[0]]->modelMatrix);
                model->Draw(0, overrideMat);
            }
            else {
                shader->setBool("useInstance", true);
                RenderInstanced(model, indices, overrideMat);
            }
        }
    }

    void RenderInstanced(Model* model, std::vector<size_t>& indices, Material* overrideMat)
    {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);

        size_t count = indices.size();
        std::vector<glm::mat4> matrices(count);

        for (size_t i = 0; i < count; i++) {
            matrices[i] = transforms[indices[i]]->modelMatrix;
        }

        model->PrepareInstancing();

        glBindBuffer(GL_ARRAY_BUFFER, model->instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::mat4), matrices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        model->Draw((GLsizei)count, overrideMat);
    }
};

#endif