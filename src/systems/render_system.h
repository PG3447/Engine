#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/ecs.h"

#include <model.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "skybox_renderer.h"
#include "../utils/camera_helper.h"
#include "../utils/light_helper.h"



class RenderSystem : public System {
private:
    using GroupKey = std::tuple<Model*, Material*>;

    struct group_hash {
        std::size_t operator()(const GroupKey& k) const {
            return std::hash<Model*>()(std::get<0>(k)) ^
                (std::hash<Material*>()(std::get<1>(k)) << 1);
        }
    };

    Query<TransformComponent, RenderComponent>* renderQuery;
    Query<TransformComponent, LightComponent>* lightQuery;
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

    struct GpuQuery {
        GLuint queries[2];
        int current = 0;
        float lastResult = 0.0f;  // cache wyniku

        void begin() {
            glBeginQuery(GL_TIME_ELAPSED, queries[current]);
        }

        void end() {
            glEndQuery(GL_TIME_ELAPSED);
        }

        void nextFrame() {
            int prev = current;
            current = (current + 1) % 2;

            // Wymuś odczyt — może lekko stallować ale zawsze aktualny
            GLuint64 time = 0;
            glGetQueryObjectui64v(queries[prev], GL_QUERY_RESULT, &time);
            lastResult = time / 1000000.0f;
        }

        float getLastResult() const {
            return lastResult;
        }
    };
    GpuQuery gpuQuery;

    struct RenderStats {
        int drawCalls = 0;
        int renderedObjects = 0;
        int triangles = 0;
        int stateChanges = 0;
        float cullingTimeMs = 0.0f;
        float drawSubmitTimeMs = 0.0f;

        void Reset() {
            drawCalls = renderedObjects = triangles = stateChanges = 0;
            cullingTimeMs = drawSubmitTimeMs = 0.0f;
        }
    };
    RenderStats stats;

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
        lightQuery = ecs.CreateQuery<TransformComponent, LightComponent>();
        cameraQuery = ecs.CreateQuery<TransformComponent, CameraComponent>();

        Init();
    }

    void Init() {
        glEnable(GL_DEPTH_TEST);
        skybox.Init();

        glGenQueries(2, gpuQuery.queries);

        for (int i = 0; i < 2; i++) {
            glBeginQuery(GL_TIME_ELAPSED, gpuQuery.queries[i]);
            glEndQuery(GL_TIME_ELAPSED);
        }

        GLuint available = 0;
        while (!available) {
            glGetQueryObjectuiv(gpuQuery.queries[0],
                               GL_QUERY_RESULT_AVAILABLE,
                               &available);
        }
    }

    void OnGameObjectUpdated(GameObject* e) override {
        renderQuery->OnGameObjectUpdated(e); // forward do query
        lightQuery->OnGameObjectUpdated(e);  // forward do query
        cameraQuery->OnGameObjectUpdated(e); // forward do query

        groupsDirty = true;
    }

    void MarkDirty() {
        groupsDirty = true;
    }

    void Update(ECS& ecs) override {
        stats.Reset();
        gpuQuery.begin();

        // OpenGL Rendering code goes here
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        BuildGroups();

        RenderAllCameras();

        gpuQuery.end();
        gpuQuery.nextFrame();
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
        Light();
        RenderGroups(frustum);

        glBindVertexArray(0);

        skybox.Render(view, projection);
    }
    
    void Light() {
        auto& renderers = std::get<1>(renderQuery->componentsVectors);
        
        auto& transforms = std::get<0>(lightQuery->componentsVectors);
        auto& lights = std::get<1>(lightQuery->componentsVectors);


        for (size_t i = 0; i < renderQuery->gameobjects.size(); i++) {

            for (size_t j = 0; j < lightQuery->gameobjects.size(); j++) {

                LightHelper::Apply(*transforms[j], *lights[j], *renderers[i]->materialOverride->shader);
            }
        }
        
     
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

            GroupKey key = { r->model, r->materialOverride.get() };
            instancedGroups[key].push_back(i);
        }
        groupsDirty = false;
    }

    void RenderGroups(const Frustum& frustum) {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        for (auto& [key, indices] : instancedGroups) {
            Model* model = std::get<0>(key);
            Material* overrideMat = std::get<1>(key);
            Shader* shader = overrideMat->shader;

            std::vector<size_t> visible;
            // Culling
            auto cullStart = std::chrono::high_resolution_clock::now();
            for (size_t i : indices)
            {
                glm::vec3 pos = glm::vec3(transforms[i]->modelMatrix[3]);
                float radius = 1.0f; // na start

                if (SphereInFrustum(frustum, pos, radius))
                {
                    visible.push_back(i);
                }
            }
            auto cullEnd = std::chrono::high_resolution_clock::now();
            stats.cullingTimeMs += std::chrono::duration<float, std::milli>(cullEnd - cullStart).count();


            if (visible.empty())
                continue;

            shader->use();
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);

            shader->setVec3("viewPos", currentCameraPos);
      
            //shader->setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
            //shader->setVec3("dirLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
            //shader->setVec3("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
            //shader->setVec3("dirLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

            if (visible.size() == 1) {
                shader->setBool("useInstance", false);
                shader->setMat4("model", transforms[visible[0]]->modelMatrix);
                auto drawStart = std::chrono::high_resolution_clock::now();
                model->Draw(0, overrideMat);
                stats.drawCalls++;
                stats.renderedObjects += visible.size();
                stats.stateChanges++;
                stats.triangles += model->GetTriangleCount() * visible.size();
                auto drawEnd = std::chrono::high_resolution_clock::now();
                stats.drawSubmitTimeMs += std::chrono::duration<float, std::milli>(drawEnd - drawStart).count();
            }
            else {
                shader->setBool("useInstance", true);
                auto drawStart = std::chrono::high_resolution_clock::now();
                RenderInstanced(model, visible, overrideMat);
                auto drawEnd = std::chrono::high_resolution_clock::now();
                stats.drawSubmitTimeMs += std::chrono::duration<float, std::milli>(drawEnd - drawStart).count(); // ← dodaj
                stats.drawCalls++;
                stats.renderedObjects += (int)visible.size();
                stats.stateChanges++;
                stats.triangles += model->GetTriangleCount() * (int)visible.size();
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