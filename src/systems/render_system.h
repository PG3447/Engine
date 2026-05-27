#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/ecs.h"

#include <model.h>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include "DebugDrawSystem.h"
#include "skybox_renderer.h"
#include "../utils/camera_helper.h"
#include "../utils/light_helper.h"
#include <glm/gtc/type_ptr.hpp>
#include "../compute_shader.h"
#include "GPUdriven_manager.h"

class RenderSystem : public System {
private:
    using GroupKey = std::tuple<RenderMesh*, Material*>;

    struct group_hash {
        std::size_t operator()(const GroupKey& k) const {
            return std::hash<RenderMesh*>()(std::get<0>(k)) ^
                (std::hash<Material*>()(std::get<1>(k)) << 1);
        }
    };

    struct OcclusionData {
        GLuint queryId = 0;
        bool isVisible = true;
        bool queryActive = false;
        int    hiddenFrames = 0;
        static constexpr int HIDE_THRESHOLD = 3;
    };
    std::unordered_map<size_t, OcclusionData> occlusionMap;
    float occluderThreshold = 3.0f;
public:
    Query<TransformComponent, RenderComponent>* renderQuery;
    private:
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

    GPUDrivenManager drivenManager;

    //GLuint sceneFBO;
    //GLuint sceneColorTexture;
    //GLuint sceneDepthRBO;
    //int fboWidth = 0, fboHeight = 0;

public:
    void IssueOcclusionQuery(size_t entityIdx, const glm::mat4& modelMatrix, const AABB& localAABB) {
        OcclusionData& data = occlusionMap[entityIdx];
        if (data.queryId == 0) glGenQueries(1, &data.queryId);

        const glm::vec3 localCorners[8] = {
            {localAABB.min.x, localAABB.min.y, localAABB.min.z},
            {localAABB.max.x, localAABB.min.y, localAABB.min.z},
            {localAABB.min.x, localAABB.max.y, localAABB.min.z},
            {localAABB.max.x, localAABB.max.y, localAABB.min.z},
            {localAABB.min.x, localAABB.min.y, localAABB.max.z},
            {localAABB.max.x, localAABB.min.y, localAABB.max.z},
            {localAABB.min.x, localAABB.max.y, localAABB.max.z},
            {localAABB.max.x, localAABB.max.y, localAABB.max.z},
        };

        glm::vec3 worldMin(FLT_MAX), worldMax(-FLT_MAX);
        for (const auto& c : localCorners) {
            glm::vec3 w = glm::vec3(modelMatrix * glm::vec4(c, 1.0f));
            worldMin = glm::min(worldMin, w);
            worldMax = glm::max(worldMax, w);
        }

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);

        glBeginQuery(GL_ANY_SAMPLES_PASSED, data.queryId);
        DebugDrawSystem::DrawAABBSolid(worldMin, worldMax, projection * view);
        glEndQuery(GL_ANY_SAMPLES_PASSED);

        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        data.queryActive = true;
    }

    bool frustumCullingEnabled = true;
    bool occlusionCullingEnabled = true;
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
        std::unordered_set<size_t> frustumCulledSet;
        std::unordered_set<size_t> occlusionCulledSet;
        float cullingTimeMs = 0.0f;
        float drawSubmitTimeMs = 0.0f;

        void Reset() {
            drawCalls = renderedObjects = triangles = stateChanges = 0;
           frustumCulledSet.clear();
            occlusionCulledSet.clear();
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

    /*bool SphereInFrustum(const Frustum& f, glm::vec3 pos, float radius)
    {
        for (int i = 0; i < 6; i++)
        {
            float distance =
                glm::dot(f.planes[i].normal, pos) + f.planes[i].d;

            if (distance < -radius)
                return false;
        }
        return true;
    }*/
    bool AABBInFrustum(const Frustum& f, const AABB& aabb, const glm::mat4& modelMatrix)
    {
        glm::vec3 corners[8] = {
            {aabb.min.x, aabb.min.y, aabb.min.z},
            {aabb.max.x, aabb.min.y, aabb.min.z},
            {aabb.min.x, aabb.max.y, aabb.min.z},
            {aabb.max.x, aabb.max.y, aabb.min.z},
            {aabb.min.x, aabb.min.y, aabb.max.z},
            {aabb.max.x, aabb.min.y, aabb.max.z},
            {aabb.min.x, aabb.max.y, aabb.max.z},
            {aabb.max.x, aabb.max.y, aabb.max.z},
        };

        for (int p = 0; p < 6; p++) {
            int outside = 0;
            for (int c = 0; c < 8; c++) {
                glm::vec3 world = glm::vec3(modelMatrix * glm::vec4(corners[c], 1.0f));
                if (glm::dot(f.planes[p].normal, world) + f.planes[p].d < 0)
                    outside++;
            }
            if (outside == 8) return false;
        }
        return true;
    }

    RenderSystem(ECS& ecs, GLFWwindow* win) : window(win)
    {
        renderQuery = ecs.CreateQuery<TransformComponent, RenderComponent>();
        lightQuery = ecs.CreateQuery<TransformComponent, LightComponent>();
        cameraQuery = ecs.CreateQuery<TransformComponent, CameraComponent>();

        drivenManager = GPUDrivenManager();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        drivenManager.Init(display_w, display_h);

        Init();
        DebugDrawSystem::Init();
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

    void Update(ECS& ecs, float dt) override {
        stats.Reset();
        gpuQuery.begin();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        InitFBO(display_w, display_h);

        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        BuildGroups();
        RenderAllCameras();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

        if (!gpuRendererInitialized && renderQuery->gameobjects.size() > 50) {
            //InitGPUDrivenRenderer(display_w, display_h);
            drivenManager.InitPassesFromScene(*renderQuery);
            gpuRendererInitialized = true;
            gpuRendererReady = true;
        }
        if (gpuRendererReady) {
            auto& lightTransforms = std::get<0>(lightQuery->componentsVectors);
            auto& lights = std::get<1>(lightQuery->componentsVectors);

            drivenManager.UpdateAndUploadLights(lights, lightTransforms);
        }
        for (size_t i = 0; i < cameras.size(); i++) {
            if (!cameras[i]->isActive)
                continue;

            RenderCamera(*cameras[i], *transforms[i], display_w, display_h);
        }
    }

    //GPUDrivenRenderer gpuRenderer;
    bool gpuRendererReady = false;
    GLuint depthTexturePrev = 0;
    GLuint depthFBO = 0;

    //std::unordered_map<AnimatorComponent*, uint32_t> animatorIDMap;

    //void InitGPUDrivenRenderer(int width, int height)
    //{
    //    auto& renderers = std::get<1>(renderQuery->componentsVectors);
    //    gpuRenderer.Init(width, height);

    //    for (size_t i = 0; i < renderers.size(); i++) {
    //        RenderComponent* r = renderers[i];
    //        if (!r) continue;

    //        if (r->animator && animatorIDMap.find(r->animator) == animatorIDMap.end())
    //            animatorIDMap[r->animator] = (uint32_t)animatorIDMap.size();

    //        for (auto& mesh : r->meshes) {
    //            if (!mesh.gpuMesh || !mesh.material || !mesh.cpuData) continue;

    //            MeshData* md = mesh.cpuData.get();
    //            Material* mat = mesh.material.get();

    //            gpuRenderer.RegisterMesh(md);
    //            gpuRenderer.RegisterMaterial(mat);
    //        }
    //    }


    //    gpuRenderer.UploadMeshes();
    //    gpuRenderer.UploadMaterials();

    //    // Depth texture — tworzona TYLKO RAZ
    //    if (depthTexturePrev == 0) {
    //        glGenTextures(1, &depthTexturePrev);
    //        glBindTexture(GL_TEXTURE_2D, depthTexturePrev);
    //        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, width, height);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //        glBindTexture(GL_TEXTURE_2D, 0);
    //    }


    //    gpuRendererReady = true;
    //}
    //std::vector<RenderData> renderDataCache;
    //std::vector<glm::mat4> boneMatricesCache;
    //struct AnimCache { uint32_t slot; AnimatorComponent* anim; };

    //std::vector<RenderData>& CollectRenderData()
    //{
    //    auto& transforms = std::get<0>(renderQuery->componentsVectors);
    //    auto& renderers = std::get<1>(renderQuery->componentsVectors);

    //    renderDataCache.clear();
    //    renderDataCache.reserve(renderQuery->gameobjects.size());

    //    const size_t objectCount = renderQuery->gameobjects.size();

    //    // trzeba znalezc to co ustawia animatory w RenderComponent zeby pozbyc sie tej petli i robic to raz
    //    // 1. najpierw zarejestruj nowe animatory
    //    for (size_t i = 0; i < objectCount; ++i) {
    //        const RenderComponent* r = renderers[i];
    //        if (!r || !r->animator) continue;
    //        if (animatorIDMap.find(r->animator) == animatorIDMap.end())
    //            animatorIDMap[r->animator] = (uint32_t)animatorIDMap.size();
    //    }

    //    size_t requiredSize = animatorIDMap.size() * MAX_BONES_PER_SKELETON;
    //    if (boneMatricesCache.size() != requiredSize)
    //        boneMatricesCache.resize(requiredSize, glm::mat4(1.0f));

    //    for (size_t i = 0; i < objectCount; ++i)
    //    {
    //        const TransformComponent* t = transforms[i];
    //        const RenderComponent* r = renderers[i];

    //        if (!t || !r)
    //            continue;
    //        const glm::mat4 model = t->modelMatrix;

    //        
    //        const auto& meshes = r->meshes;
    //        auto animator = r->animator;

    //        auto animIt = animator ? animatorIDMap.find(animator) : animatorIDMap.end();
    //        if (animIt != animatorIDMap.end() && animator->currentSkeleton)
    //        {
    //            uint32_t slot = animIt->second;
    //            uint32_t boneCount = (uint32_t)std::min(animator->finalBoneMatrices.size(), (size_t)MAX_BONES_PER_SKELETON);

    //            std::copy(animator->finalBoneMatrices.begin(), animator->finalBoneMatrices.begin() + boneCount, boneMatricesCache.begin() + slot * MAX_BONES_PER_SKELETON);
    //        }

    //        for (const auto& mesh : meshes)
    //        {
    //            auto* gpuMesh = mesh.cpuData.get();
    //            auto* material = mesh.material.get();

    //            if (!gpuMesh || !material || !mesh.cpuData)
    //                continue;

    //            auto meshID = gpuRenderer.GetMeshId(gpuMesh);
    //            if (meshID == UINT32_MAX)
    //                continue;

    //            auto matID = gpuRenderer.GetMaterialId(material);
    //            if (matID == UINT32_MAX)
    //                continue;


    //            auto& aabb = mesh.cpuData->aabb;

    //            renderDataCache.emplace_back(RenderData{
    //                .modelMatrix = model,
    //                .aabbMin = glm::vec4(aabb.min, 0.0f),
    //                .aabbMax = glm::vec4(aabb.max, 0.0f),
    //                .meshID = meshID,
    //                .materialID = matID,
    //                .skeletonID = animIt != animatorIDMap.end() ? animIt->second : NO_SKELETON,
    //                .padding = 0
    //                });            
    //        }
    //    }
    //    
    //    gpuRenderer.ResizeBoneBufferIfNeeded((uint32_t)animatorIDMap.size());
    //    gpuRenderer.UploadAllBoneMatrices(boneMatricesCache);

    //    return renderDataCache;
    //}

    void RenderCameraGPUDriven(CameraComponent& cam, TransformComponent& transform, int width, int height)
    {
        ApplyViewport(cam.viewport, width, height);

        view = CameraHelper::getViewMatrix(cam, transform);
        projection = CameraHelper::getProjectionMatrix(cam, width, height);

        glm::mat4 vp = projection * view;
        currentCameraPos = transform.position;

        if (gpuRendererReady) {

            //std::vector<RenderData> objects = CollectRenderData();

        /*    gpuRenderer.shaderRender->use();
            gpuRenderer.shaderRender->setMat4("viewProjection", vp);
            gpuRenderer.shaderRender->setVec3("viewPos", currentCameraPos);
            gpuRenderer.shaderRender->setBool("isAnimated", false);*/
            // + światła jak w starym kodzie...
            drivenManager.CollectAllPasses(*renderQuery, currentCameraPos);

            drivenManager.RenderFrame(vp, currentCameraPos, depthTexturePrev);

            // Skopiuj depth bieżącej klatki do depthTexturePrev dla następnej
       /*     std::vector<float> zeros(width * height, 0.0f);
            glTextureSubImage2D(depthTexturePrev, 0, 0, 0, width, height,
                GL_DEPTH_COMPONENT, GL_FLOAT, zeros.data());*/
        }

        DebugDrawSystem::Flush(vp);

        glBindVertexArray(0);
        
        skybox.Render(view, projection);
    }


    void RenderCamera(CameraComponent& cam, TransformComponent& transform, int width, int height) {
        ApplyViewport(cam.viewport, width, height);

        view = CameraHelper::getViewMatrix(cam, transform);
        projection = CameraHelper::getProjectionMatrix(cam, width, height);

        glm::mat4 vp = projection * view;
        Frustum frustum = ExtractFrustum(vp);

        currentCameraPos = transform.position;
        RenderGroups(frustum);

        DebugDrawSystem::Flush(vp);


        glBindVertexArray(0);

        skybox.Render(view, projection);
    }

    void BuildGroups() {
        if (!groupsDirty) return;
        auto& renderers = std::get<1>(renderQuery->componentsVectors);
        instancedGroups.clear();

        for (size_t i = 0; i < renderQuery->gameobjects.size(); i++) {
            RenderComponent* r = renderers[i];
            if (!r) continue;

            for (auto& mesh : r->meshes)
            {
                if (!mesh.gpuMesh || !mesh.material)
                    continue;

                GroupKey key = {
                    mesh.gpuMesh.get(),
                    mesh.material.get()
                };

                instancedGroups[key].push_back(i);
            }
        }

        groupsDirty = false;
    }


    //if (!r || !r->model) continue;

    //if (r->shader) {
    //    r->model->SetShader(r->shader);
    //}

    //GroupKey key = { r->model, r->materialOverride.get() };
    //instancedGroups[key].push_back(i);
    //    }


    void RenderGroups(const Frustum& frustum) {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        auto& renderers = std::get<1>(renderQuery->componentsVectors);
        auto& transformsLights = std::get<0>(lightQuery->componentsVectors);
        auto& lights = std::get<1>(lightQuery->componentsVectors);


        std::unordered_map<size_t, AABB> allSubjects; // entityIdx -> local AABB, do debugowania occlusion culling

        for (auto& [key, indices] : instancedGroups) {
            RenderMesh* model = std::get<0>(key);
            Material* overrideMat = std::get<1>(key);
            Shader* shader = overrideMat->shader;

            if (shader == nullptr)
                continue;


            std::vector<size_t> occluders;
            std::vector<size_t> subjects;
            // Culling
            auto cullStart = std::chrono::high_resolution_clock::now();
            for (size_t i : indices) {
                AABB localAABB = GetLocalAABB(renderers[i]->meshes);

                if (!AABBInFrustum(frustum, localAABB, transforms[i]->modelMatrix)) {
                    if (frustumCullingEnabled) {
                        stats.frustumCulledSet.insert(i);
                    }
                    continue;
                }

                glm::vec3 size = localAABB.max - localAABB.min;

                float dims[3] = { size.x, size.y, size.z };
                std::sort(dims, dims + 3);

                if (size.x * size.y * size.z > occluderThreshold) {
                    occluders.push_back(i);
                } else {
                    subjects.push_back(i);
                    allSubjects.emplace(i, localAABB);
                }
            }
            auto cullEnd = std::chrono::high_resolution_clock::now();
            stats.cullingTimeMs += std::chrono::duration<float, std::milli>(cullEnd - cullStart).count();

            std::vector<size_t> visibleSubjects;

            if (occlusionCullingEnabled) {
                for (size_t i : subjects) {
                    OcclusionData& data = occlusionMap[i];

                    if (data.queryId != 0 && data.queryActive) {
                        GLuint available = 0;
                        glGetQueryObjectuiv(data.queryId, GL_QUERY_RESULT_AVAILABLE, &available);
                        if (available) {
                            GLuint anyPassed = 0;
                            glGetQueryObjectuiv(data.queryId, GL_QUERY_RESULT, &anyPassed);
                            if (anyPassed > 0) {
                                data.isVisible = true;
                                data.hiddenFrames = 0; // reset licznika
                            } else {
                                data.hiddenFrames++;
                                if (data.hiddenFrames >= OcclusionData::HIDE_THRESHOLD)
                                    data.isVisible = false; // ukryj dopiero po N klatkach
                            }
                            data.queryActive = false;
                        }
                    } else {
                        data.isVisible = true;
                    }

                    if (data.isVisible) {
                        visibleSubjects.push_back(i);
                    } else {
                        stats.occlusionCulledSet.insert(i);
                    }
                }
            } else {
                visibleSubjects = subjects;
            }

            std::vector<size_t> finalToRender = occluders;
            finalToRender.insert(finalToRender.end(), visibleSubjects.begin(), visibleSubjects.end());

            if (finalToRender.empty()) continue;

            shader->use();
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);
            shader->setVec3("viewPos", currentCameraPos);
            
            int numPointLight = 0;
            int numSpotLight = 0;

            // light
            for (size_t i = 0; i < lights.size(); i++)
            {
                if (lights[i]->type == Point) 
                {
                    numPointLight++;
                }

                if (lights[i]->type == Spot) 
                {
                    numSpotLight++;
                }
            }

            LightHelper::ApplyNumberLight(numPointLight, numSpotLight, *shader);

            for (size_t i = 0; i < lightQuery->gameobjects.size(); i++)
            {
                LightHelper::Apply(*transformsLights[i], *lights[i], *shader);
            }

            //shader->setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
            //shader->setVec3("dirLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
            //shader->setVec3("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
            //shader->setVec3("dirLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

            std::vector<size_t> standardVisible;
            std::vector<size_t> animatedVisible;

            for (size_t i : finalToRender) {
                auto* go = renderQuery->gameobjects[i];
                AnimatorComponent* animator = go->template GetComponent<AnimatorComponent>();
                GameObject* current = go->GetParent();
                while (animator == nullptr && current != nullptr) {
                    animator = current->template GetComponent<AnimatorComponent>();
                    current = current->GetParent();
                }

                if (animator) animatedVisible.push_back(i);
                else          standardVisible.push_back(i);
            }

            if (!standardVisible.empty()) {
                shader->setBool("isAnimated", false);

                if (standardVisible.size() == 1) {
                    shader->setBool("useInstance", false);
                    shader->setMat4("model", transforms[standardVisible[0]]->modelMatrix);
                    auto drawStart = std::chrono::high_resolution_clock::now();
                    overrideMat->Apply();
                    model->Draw(0);
                    stats.drawCalls++;
                    stats.renderedObjects++;
                    stats.stateChanges++;
                    stats.triangles += GetTriangleCount(model);
                    auto drawEnd = std::chrono::high_resolution_clock::now();
                    stats.drawSubmitTimeMs += std::chrono::duration<float, std::milli>(drawEnd - drawStart).count();
                }
                else {
                    shader->setBool("useInstance", true);
                    auto drawStart = std::chrono::high_resolution_clock::now();
                    RenderInstanced(model, standardVisible, overrideMat);
                    auto drawEnd = std::chrono::high_resolution_clock::now();
                    stats.drawSubmitTimeMs += std::chrono::duration<float, std::milli>(drawEnd - drawStart).count();
                    stats.drawCalls++;
                    stats.renderedObjects += (int)standardVisible.size();
                    stats.stateChanges++;
                    stats.triangles += GetTriangleCount(model) * (int)standardVisible.size();
                }
            }

            if (!animatedVisible.empty()) {
                shader->setBool("useInstance", false);
                shader->setBool("isAnimated", true);

                for (size_t i : animatedVisible) {
                    auto* go = renderQuery->gameobjects[i];
                    auto* renderComp = std::get<1>(renderQuery->componentsVectors)[i];

                    AnimatorComponent* animator = go->template GetComponent<AnimatorComponent>();

                    GameObject* current = go->GetParent();
                    while (animator == nullptr && current != nullptr) {
                        animator = current->template GetComponent<AnimatorComponent>();
                        current = current->GetParent();
                    }
                    //if (!animator && renderComp->rootAnimator) {
                    //    animator = renderComp->rootAnimator;
                    //}

                    shader->setMat4("model", transforms[i]->modelMatrix);

                    if (animator && animator->currentSkeleton) {
                        shader->setMat4Array("finalBonesMatrices", animator->finalBoneMatrices);
                    }

                    auto drawStart = std::chrono::high_resolution_clock::now();
                    overrideMat->Apply();
                    model->Draw(0);
                    stats.drawCalls++;
                    stats.renderedObjects++;
                    stats.stateChanges++;
                    stats.triangles += GetTriangleCount(model);
                    auto drawEnd = std::chrono::high_resolution_clock::now();
                    stats.drawSubmitTimeMs += std::chrono::duration<float, std::milli>(drawEnd - drawStart).count();
                }
            }
        }
        if (occlusionCullingEnabled) {
            for (auto& [i, localAABB] : allSubjects) {
                IssueOcclusionQuery(i, transforms[i]->modelMatrix, localAABB);
            }
        }
    }

    void RenderInstanced(RenderMesh* model, std::vector<size_t>& indices, Material* overrideMat)
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

        overrideMat->Apply();
        model->Draw((GLsizei)count);
    }

    AABB GetLocalAABB(vector<MeshNode> meshes) const
    {
        AABB result;
        for (auto& node : meshes) {
            result.min = glm::min(result.min, node.aabb.min);
            result.max = glm::max(result.max, node.aabb.max);
        }
        return result;
    }

    int GetTriangleCount(RenderMesh* mesh) const
    {
        int total = 0;

        total += mesh->indicesCount / 3;


        return total;
    }


    void InitFBO(int w, int h) {
        if (fboWidth == w && fboHeight == h) return; // bez zmian
        fboWidth = w; fboHeight = h;

        if (sceneFBO) {
            glDeleteFramebuffers(1, &sceneFBO);
            glDeleteTextures(1, &sceneColorTexture);
            glDeleteRenderbuffers(1, &sceneDepthRBO);
        }

        glGenFramebuffers(1, &sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        // Textura koloru
        glGenTextures(1, &sceneColorTexture);
        glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTexture, 0);

        // Renderbuffer dla depth+stencil
        glGenRenderbuffers(1, &sceneDepthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            spdlog::error("PostProcessing FBO incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint GetSceneTexture() const { return sceneColorTexture; }

};

#endif