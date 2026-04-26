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
    struct Plane {
        glm::vec3 normal;
        float d;
    };

    struct Frustum {
        Plane planes[6];
    };

    Frustum ExtractFrustum(const glm::mat4& vp)
    {
        Frustum f;

        // LEFT
        f.planes[0].normal.x = vp[0][3] + vp[0][0];
        f.planes[0].normal.y = vp[1][3] + vp[1][0];
        f.planes[0].normal.z = vp[2][3] + vp[2][0];
        f.planes[0].d       = vp[3][3] + vp[3][0];

        // RIGHT
        f.planes[1].normal.x = vp[0][3] - vp[0][0];
        f.planes[1].normal.y = vp[1][3] - vp[1][0];
        f.planes[1].normal.z = vp[2][3] - vp[2][0];
        f.planes[1].d        = vp[3][3] - vp[3][0];

        // TOP
        f.planes[2].normal.x = vp[0][3] - vp[0][1];
        f.planes[2].normal.y = vp[1][3] - vp[1][1];
        f.planes[2].normal.z = vp[2][3] - vp[2][1];
        f.planes[2].d        = vp[3][3] - vp[3][1];

        // BOTTOM
        f.planes[3].normal.x = vp[0][3] + vp[0][1];
        f.planes[3].normal.y = vp[1][3] + vp[1][1];
        f.planes[3].normal.z = vp[2][3] + vp[2][1];
        f.planes[3].d        = vp[3][3] + vp[3][1];

        // NEAR
        f.planes[4].normal.x = vp[0][3] + vp[0][2];
        f.planes[4].normal.y = vp[1][3] + vp[1][2];
        f.planes[4].normal.z = vp[2][3] + vp[2][2];
        f.planes[4].d        = vp[3][3] + vp[3][2];

        // FAR
        f.planes[5].normal.x = vp[0][3] - vp[0][2];
        f.planes[5].normal.y = vp[1][3] - vp[1][2];
        f.planes[5].normal.z = vp[2][3] - vp[2][2];
        f.planes[5].d        = vp[3][3] - vp[3][2];

        for (int i = 0; i < 6; i++)
        {
            float len = glm::length(f.planes[i].normal);
            f.planes[i].normal /= len;
            f.planes[i].d /= len;
        }

        return f;
    }

    bool SphereInFrustum(const Frustum& f, glm::vec3 pos, float radius)
    {
        for (int i = 0; i < 6; i++)
        {
            float distance =
                glm::dot(f.planes[i].normal, pos) + f.planes[i].d;

            if (distance < -radius)
                return false;
        }
        return true;
    }
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

        glm::mat4 vp = projection * view;
        Frustum frustum = ExtractFrustum(vp);

        currentCameraPos = transform.position;

        RenderGroups(frustum);

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

    void RenderGroups(const Frustum& frustum) {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        for (auto& [key, indices] : instancedGroups) {
            Model* model = std::get<0>(key);
            Shader* shader = std::get<1>(key);
            Material* overrideMat = std::get<2>(key);

            std::vector<size_t> visible;
            for (size_t i : indices)
            {
                glm::vec3 pos = glm::vec3(transforms[i]->modelMatrix[3]);
                float radius = 1.0f; // na start

                if (SphereInFrustum(frustum, pos, radius))
                {
                    visible.push_back(i);
                }
            }

            if (visible.empty())
                continue;

            shader->use();
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);

            shader->setVec3("viewPos", currentCameraPos);
            shader->setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
            shader->setVec3("dirLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
            shader->setVec3("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
            shader->setVec3("dirLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

            if (visible.size() == 1) {
                shader->setBool("useInstance", false);
                shader->setMat4("model", transforms[visible[0]]->modelMatrix);
                model->Draw(0, overrideMat);
            }
            else {
                shader->setBool("useInstance", true);
                RenderInstanced(model, visible, overrideMat);
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