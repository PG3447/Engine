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
#include "../utils/render_helper.h"
#include <glm/gtc/type_ptr.hpp>
#include "../compute_shader.h"
#include "GPUdriven_renderer.h"

class RenderSystem : public System {
private:
    using GroupKey = std::tuple<RenderMesh*, Material*>;

    struct group_hash {
        std::size_t operator()(const GroupKey& k) const {
            return std::hash<RenderMesh*>()(std::get<0>(k)) ^
                (std::hash<Material*>()(std::get<1>(k)) << 1);
        }
    };

    //struct GroupKeyHash {
    //    size_t operator()(const GroupKey& k) const {
    //        return std::hash<void*>()(std::get<0>(k))
    //            ^ (std::hash<void*>()(std::get<1>(k)) << 1);
    //    }
    //};


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
    Query<TransformComponent, RenderComponent, AnimatorComponent>* animatorQuery;


    std::unordered_map<GroupKey, std::vector<size_t>, group_hash> instancedGroupsOpaque;
    //std::unordered_map<GroupKey, std::vector<TransparentMesh>, group_hash> instancedGroupsTransparent;

    struct TransparentMesh
    {
        size_t index;
        float distance;
    };

    struct InsancedTransparent
    {
        GroupKey key;
        std::vector<TransparentMesh> objects;
    };

    std::unordered_map<GroupKey, size_t, group_hash> instancedTransparentLookup;
    std::vector<InsancedTransparent> instancedTransparentVector;

    //using GroupKey = std::tuple<RenderMesh*, Material*>;


    //std::vector<TransparentGroup> groups;
    //std::unordered_map<GroupKey, size_t, GroupKeyHash> lookup;

    bool groupsDirty = true;

    GLFWwindow* window = nullptr;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    GLuint texture;

    SkyboxRenderer skybox;

    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 currentCameraPos;

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

        glm::vec4 color = data.isVisible
            ? glm::vec4(0, 1, 0, 1)  // zielony = widoczny
            : glm::vec4(1, 0, 0, 1); // czerwony = culled
        DebugDrawSystem::AddAABB(worldMin, worldMax, color);
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
        animatorQuery = ecs.CreateQuery<TransformComponent, RenderComponent, AnimatorComponent>();

        Init();
        DebugDrawSystem::Init();
    }

    void Init() {
        gpuRenderer = GPUDrivenRenderer();
        gpuRenderer.shaderHizCullCount = new ComputeShader("res/shaders/hiz_culling_count.comp");
        gpuRenderer.shaderPrefixSum = new ComputeShader("res/shaders/prefix_sum.comp");
        gpuRenderer.shaderHizWritePass = new ComputeShader("res/shaders/write_pass.comp");
        gpuRenderer.shaderBuildCmds = new ComputeShader("res/shaders/build_commands.comp");
        gpuRenderer.shaderHizDownsample = new ComputeShader("res/shaders/hiz_build.comp");
        gpuRenderer.shaderRender = new Shader("res/shaders/gpu_driven.vert", "res/shaders/gpu_driven.frag");

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
        animatorQuery->OnGameObjectUpdated(e);

        groupsDirty = true;
    }

    void MarkDirty()
    {
        groupsDirty = true;
    }

    void Update(ECS& ecs, float dt) override {
        stats.Reset();
        gpuQuery.begin();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        //InitFBO(display_w, display_h);

        //glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //BuildGroups();
        RenderAllCameras();

        //glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
    bool gpuRendererInitialized = false;

    void RenderAllCameras() {
        auto& transforms = std::get<0>(cameraQuery->componentsVectors);
        auto& cameras = std::get<1>(cameraQuery->componentsVectors);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        if (!gpuRendererInitialized && renderQuery->gameobjects.size() > 50) {
            InitGPUDrivenRenderer(display_w, display_h);
            gpuRendererInitialized = true;
        }
        if (gpuRendererReady) {
            auto& lightTransforms = std::get<0>(lightQuery->componentsVectors);
            auto& lights = std::get<1>(lightQuery->componentsVectors);

            gpuRenderer.UpdateAndUploadLights(lights, lightTransforms);
        }
        for (size_t i = 0; i < cameras.size(); i++) {
            if (!cameras[i]->isActive)
                continue;
            RenderCameraGPUDriven(*cameras[i], *transforms[i], display_w, display_h);
            //RenderCamera(*cameras[i], *transforms[i], display_w, display_h);
        }
    }

    GPUDrivenRenderer gpuRenderer;
    bool gpuRendererReady = false;
    GLuint depthTexturePrev = 0;
    GLuint depthFBO = 0;

    //std::unordered_map<MeshData*, uint32_t>  meshIDMap;
    //std::unordered_map<Material*, uint32_t>  materialIDMap;
    std::unordered_map<AnimatorComponent*, uint32_t> animatorIDMap;

    void InitGPUDrivenRenderer(int width, int height)
    {
        auto& renderers = std::get<1>(renderQuery->componentsVectors);
        gpuRenderer.Init(width, height);

        for (size_t i = 0; i < renderers.size(); i++) {
            RenderComponent* r = renderers[i];
            if (!r) continue;

            if (r->animator && animatorIDMap.find(r->animator) == animatorIDMap.end())
                animatorIDMap[r->animator] = (uint32_t)animatorIDMap.size();

            for (auto& mesh : r->meshes) {
                if (!mesh.gpuMesh || !mesh.material || !mesh.cpuData) continue;

                MeshData* md = mesh.cpuData.get();
                Material* mat = mesh.material.get();

                //if (meshIDMap.find(md) == meshIDMap.end()) {
                //    meshIDMap[md] = gpuRenderer.RegisterMesh(*md);
                //}
                gpuRenderer.RegisterMesh(md);
                gpuRenderer.RegisterMaterial(mat);

                //if (materialIDMap.find(mat) == materialIDMap.end()) {
                //    materialIDMap[mat] = gpuRenderer.RegisterMaterial(mat);
                //}
            }
        }


        gpuRenderer.UploadMeshes();
        gpuRenderer.UploadMaterials();

        //auto& lightTransforms = std::get<0>(lightQuery->componentsVectors);
        //auto& lights = std::get<1>(lightQuery->componentsVectors);

        //for (size_t i = 0; i < lights.size(); i++)
        //{
        //    if (!lights[i] || !lightTransforms[i]) continue;
        //    gpuRenderer.RegisterLight(lights[i], lightTransforms[i]);
        //}
        //gpuRenderer.AllocateLightsBuffer();
        //gpuRenderer.UploadLights();
        

        // Depth texture — tworzona TYLKO RAZ
        if (depthTexturePrev == 0) {
            glGenTextures(1, &depthTexturePrev);
            glBindTexture(GL_TEXTURE_2D, depthTexturePrev);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, width, height);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
        }


        gpuRendererReady = true;
    }
    std::vector<RenderData> renderDataCache;
    std::vector<glm::mat4> boneMatricesCache;
    struct AnimCache { uint32_t slot; AnimatorComponent* anim; };

    std::vector<RenderData>& CollectRenderData()
    {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        auto& renderers = std::get<1>(renderQuery->componentsVectors);

        renderDataCache.clear();
        renderDataCache.reserve(renderQuery->gameobjects.size());

        const size_t objectCount = renderQuery->gameobjects.size();

        // trzeba znalezc to co ustawia animatory w RenderComponent zeby pozbyc sie tej petli i robic to raz
        // 1. najpierw zarejestruj nowe animatory
        for (size_t i = 0; i < objectCount; ++i) {
            const RenderComponent* r = renderers[i];
            if (!r || !r->animator) continue;
            if (animatorIDMap.find(r->animator) == animatorIDMap.end())
                animatorIDMap[r->animator] = (uint32_t)animatorIDMap.size();
        }

        size_t requiredSize = animatorIDMap.size() * MAX_BONES_PER_SKELETON;
        if (boneMatricesCache.size() != requiredSize)
            boneMatricesCache.resize(requiredSize, glm::mat4(1.0f));

        for (size_t i = 0; i < objectCount; ++i)
        {
            const TransformComponent* t = transforms[i];
            const RenderComponent* r = renderers[i];

            if (!t || !r)
                continue;
            const glm::mat4 model = t->modelMatrix;

            
            const auto& meshes = r->meshes;
            auto animator = r->animator;

            auto animIt = animator ? animatorIDMap.find(animator) : animatorIDMap.end();
            if (animIt != animatorIDMap.end() && animator->currentSkeleton)
            {
                uint32_t slot = animIt->second;
                uint32_t boneCount = (uint32_t)std::min(animator->finalBoneMatrices.size(), (size_t)MAX_BONES_PER_SKELETON);

                std::copy(animator->finalBoneMatrices.begin(), animator->finalBoneMatrices.begin() + boneCount, boneMatricesCache.begin() + slot * MAX_BONES_PER_SKELETON);
            }

            for (const auto& mesh : meshes)
            {
                auto* gpuMesh = mesh.cpuData.get();
                auto* material = mesh.material.get();

                if (!gpuMesh || !material || !mesh.cpuData)
                    continue;

                auto meshID = gpuRenderer.GetMeshId(gpuMesh);
                if (meshID == UINT32_MAX)
                    continue;

                auto matID = gpuRenderer.GetMaterialId(material);
                if (matID == UINT32_MAX)
                    continue;


                auto& aabb = mesh.cpuData->aabb;

                //glm::vec3 worldMin(FLT_MAX), worldMax(-FLT_MAX);
                //glm::vec3 corners[8] = {
                //    {aabb.min.x, aabb.min.y, aabb.min.z},
                //    {aabb.max.x, aabb.min.y, aabb.min.z},
                //    {aabb.min.x, aabb.max.y, aabb.min.z},
                //    {aabb.max.x, aabb.max.y, aabb.min.z},
                //    {aabb.min.x, aabb.min.y, aabb.max.z},
                //    {aabb.max.x, aabb.min.y, aabb.max.z},
                //    {aabb.min.x, aabb.max.y, aabb.max.z},
                //    {aabb.max.x, aabb.max.y, aabb.max.z},
                //};
                //for (const auto& c : corners) {
                //    glm::vec3 w = glm::vec3(model * glm::vec4(c, 1.0f));
                //    worldMin = glm::min(worldMin, w);
                //    worldMax = glm::max(worldMax, w);
                //}

                //DebugDrawSystem::AddAABB(worldMin, worldMax, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
                renderDataCache.emplace_back(RenderData{
                    .modelMatrix = model,
                    .aabbMin = glm::vec4(aabb.min, 0.0f),
                    .aabbMax = glm::vec4(aabb.max, 0.0f),
                    .meshID = meshID,
                    .materialID = matID,
                    .skeletonID = animIt != animatorIDMap.end() ? animIt->second : NO_SKELETON,
                    .padding = 0
                    });            
            }
        }
        
        gpuRenderer.ResizeBoneBufferIfNeeded((uint32_t)animatorIDMap.size());
        gpuRenderer.UploadAllBoneMatrices(boneMatricesCache);

        return renderDataCache;
    }

    void RenderCameraGPUDriven(CameraComponent& cam, TransformComponent& transform, int width, int height)
    {
        ApplyViewport(cam.viewport, width, height);

        view = CameraHelper::getViewMatrix(cam, transform);
        projection = CameraHelper::getProjectionMatrix(cam, width, height);

        glm::mat4 vp = projection * view;
        currentCameraPos = transform.position;

        if (gpuRendererReady) {



            std::vector<RenderData> objects = CollectRenderData();

        /*    gpuRenderer.shaderRender->use();
            gpuRenderer.shaderRender->setMat4("viewProjection", vp);
            gpuRenderer.shaderRender->setVec3("viewPos", currentCameraPos);
            gpuRenderer.shaderRender->setBool("isAnimated", false);*/
            // + światła jak w starym kodzie...

            gpuRenderer.RenderFrame(vp, objects, depthTexturePrev, currentCameraPos);

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
        auto& transfoms = std::get<0>(renderQuery->componentsVectors);
        auto& renderers = std::get<1>(renderQuery->componentsVectors);
        instancedGroupsOpaque.clear();
        instancedTransparentLookup.clear();
        instancedTransparentVector.clear();

        for (size_t i = 0; i < renderQuery->gameobjects.size(); i++) {
            TransformComponent* t = transfoms[i];
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


                //Opaque
                if (mesh.material->surfaceType == SurfaceType::Opaque)
                {
                    instancedGroupsOpaque[key].push_back(i);
                }

                //Transparent
                if (mesh.material->surfaceType == SurfaceType::Transparent)
                {
                    glm::vec3 worldCenter = glm::vec3(t->modelMatrix * glm::vec4(mesh.cpuData->aabb.centerLocal, 1.0f));
                    float distance = glm::length2(currentCameraPos - worldCenter);

                    auto it = instancedTransparentLookup.find(key);

                    if (it == instancedTransparentLookup.end())
                    {
                        size_t newIndex = instancedTransparentVector.size();
                        instancedTransparentVector.push_back({});
                        instancedTransparentVector.back().key = key;

                        instancedTransparentLookup[key] = newIndex;
                        it = instancedTransparentLookup.find(key);
                    }

                    instancedTransparentVector[it->second].objects.push_back({ i, distance });
                }
            }
        }

        //SortTransparent();

        groupsDirty = false;
    }

    void SortTransparent()
    {

        for (auto& transparentVector : instancedTransparentVector)
        {
            std::sort(transparentVector.objects.begin(), transparentVector.objects.end(),
                [](const TransparentMesh& a, const TransparentMesh& b)
                {
                    return a.distance > b.distance;
                });
        }

        std::sort(instancedTransparentVector.begin(), instancedTransparentVector.end(),
            [](const InsancedTransparent& a, const InsancedTransparent& b)
            {
                return a.objects.front().distance > b.objects.front().distance;
            });
    }



    void RenderGroups(const Frustum& frustum) {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        auto& renderers = std::get<1>(renderQuery->componentsVectors);
        auto& transformsLights = std::get<0>(lightQuery->componentsVectors);
        auto& lights = std::get<1>(lightQuery->componentsVectors);


        std::unordered_map<size_t, AABB> allSubjects; // entityIdx -> local AABB, do debugowania occlusion culling

        for (auto& [key, indices] : instancedGroupsOpaque) {
            RenderMesh* model = std::get<0>(key);
            Material* material = std::get<1>(key);
            Shader* shader = material->shader;

            if (shader == nullptr)
                continue;


            std::vector<size_t> occluders;
            std::vector<size_t> subjects;
            // Culling
            auto cullStart = std::chrono::high_resolution_clock::now();
            for (size_t i : indices) {
                AABB localAABB = renderers[i]->localObjectAABB; //RenderHelper::GetLocalAABB(renderers[i]->meshes);

                if (frustumCullingEnabled && !AABBInFrustum(frustum, localAABB, transforms[i]->modelMatrix)) {
                    stats.frustumCulledSet.insert(i);
                    continue;
                }

                glm::vec3 size = localAABB.max - localAABB.min;

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

            //Transparent
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            // light
            int numPointLight = 0;
            int numSpotLight = 0;
            
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
          
            if (finalToRender.size() == 1) {
                shader->setBool("useInstance", false);

                if (renderers[finalToRender[0]]->animator)
                {
                    AnimatorComponent* animator = renderers[finalToRender[0]]->animator;
                    shader->setBool("isAnimated", true);

                    if (animator && animator->currentSkeleton) {
                        shader->setMat4Array("finalBonesMatrices", animator->finalBoneMatrices);
                    }

                }
                else {
                    shader->setBool("isAnimated", false);
                }

                shader->setMat4("model", transforms[finalToRender[0]]->modelMatrix);
                auto drawStart = std::chrono::high_resolution_clock::now();
                material->Apply();
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
                shader->setBool("isAnimated", false);

                auto drawStart = std::chrono::high_resolution_clock::now();
                RenderInstanced(model, finalToRender, material);
                auto drawEnd = std::chrono::high_resolution_clock::now();
                stats.drawSubmitTimeMs += std::chrono::duration<float, std::milli>(drawEnd - drawStart).count();
                stats.drawCalls++;
                stats.renderedObjects += (int)finalToRender.size();
                stats.stateChanges++;
                stats.triangles += GetTriangleCount(model) * (int)finalToRender.size();
            }


        }



        if (occlusionCullingEnabled) {
            for (auto& [i, localAABB] : allSubjects) {
                IssueOcclusionQuery(i, transforms[i]->modelMatrix, localAABB);
            }
        }
    }

    void RenderInstanced(RenderMesh* model, std::vector<size_t>& indices, Material* material)
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

        material->Apply();
        model->Draw((GLsizei)count);
    }

    int GetTriangleCount(RenderMesh* mesh) const
    {
        int total = 0;

        total += mesh->indicesCount / 3;


        return total;
    }


    //void InitFBO(int w, int h) {
    //    if (fboWidth == w && fboHeight == h) return; // bez zmian
    //    fboWidth = w; fboHeight = h;

    //    if (sceneFBO) {
    //        glDeleteFramebuffers(1, &sceneFBO);
    //        glDeleteTextures(1, &sceneColorTexture);
    //        glDeleteRenderbuffers(1, &sceneDepthRBO);
    //    }

    //    glGenFramebuffers(1, &sceneFBO);
    //    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    //    // Textura koloru
    //    glGenTextures(1, &sceneColorTexture);
    //    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTexture, 0);

    //    // Renderbuffer dla depth+stencil
    //    glGenRenderbuffers(1, &sceneDepthRBO);
    //    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
    //    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    //    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRBO);

    //    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    //        spdlog::error("PostProcessing FBO incomplete!");

    //    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //}

    //GLuint GetSceneTexture() const { return sceneColorTexture; }

  

};

#endif
//
//
//struct HiZBuffer {
//    GLuint fbo = 0;
//    GLuint depthTex = 0;
//    int    width = 0;
//    int    height = 0;
//    int    mipLevels = 0;
//
//    void Init(int w, int h) {
//        width = w;
//        height = h;
//        mipLevels = (int)std::floor(std::log2(std::max(w, h))) + 1;
//
//        // Tekstura depth z mipmapy
//        glGenTextures(1, &depthTex);
//        glBindTexture(GL_TEXTURE_2D, depthTex);
//        glTexStorage2D(GL_TEXTURE_2D, mipLevels, GL_DEPTH_COMPONENT32F, w, h);
//
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//        // FBO dla mip 0
//        glGenFramebuffers(1, &fbo);
//        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
//            GL_TEXTURE_2D, depthTex, 0);
//        glDrawBuffer(GL_NONE);
//        glReadBuffer(GL_NONE);
//        glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    }
//};


//class HiZOcclusionCuller {
//public:
//    HiZBuffer hiz;
//    GLuint  hizBuildShader = 0;
//    GLuint  hizCullShader = 0;
//
//    GLuint objectSSBO = 0; // dane AABB obiektów
//    GLuint drawCmdSSBO = 0; // indirect draw commands
//    GLuint counterSSBO = 0; // licznik widocznych
//
//    void Init(int w, int h) {
//        hiz.Init(w, h);
//        hizBuildShader = ComputeShader("res/shaders/hiz_build.comp").ID;
//        hizCullShader = ComputeShader("res/shaders/hiz_culling.comp").ID;
//    }
//
//    // Krok 1 — skopiuj depth z głównego FBO do Hi-Z
//    void CopyDepth(GLuint mainDepthTex) {
//        glCopyImageSubData(mainDepthTex, GL_TEXTURE_2D, 0, 0, 0, 0, hiz.depthTex, GL_TEXTURE_2D, 0, 0, 0, 0, hiz.width, hiz.height, 1);
//    }
//
//    // Krok 2 — zbuduj mipmapy Hi-Z
//    void BuildMips() {
//        glUseProgram(hizBuildShader); // compute_shader->use
//
//        for (int mip = 1; mip < hiz.mipLevels; mip++) {
//            int mipW = std::max(1, hiz.width >> mip);
//            int mipH = std::max(1, hiz.height >> mip);
//
//            // Czytaj z poprzedniego mipa
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, hiz.depthTex);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, mip - 1);
//            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip - 1);
//
//            // Pisz do aktualnego mipa
//            glBindImageTexture(1, hiz.depthTex, mip, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
//
//            glDispatchCompute(
//                (mipW + 7) / 8,
//                (mipH + 7) / 8,
//                1
//            );
//            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
//        }
//
//        // Przywróć pełny zakres mipów
//        glBindTexture(GL_TEXTURE_2D, hiz.depthTex);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, hiz.mipLevels - 1);
//    }
//
//    // Krok 3 — culling na GPU
//    void Cull(const glm::mat4& vp, int objectCount) {
//        // Zeruj licznik
//        uint32_t zero = 0;
//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterSSBO);
//        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);
//
//        glUseProgram(hizCullShader);  // compute_shader->use
//        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(vp));
//        glUniform1i(1, objectCount);
//        glUniform1i(2, hiz.mipLevels);
//        glUniform2f(3, (float)hiz.width, (float)hiz.height);
//
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, hiz.depthTex);
//
//        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, objectSSBO);
//        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, drawCmdSSBO);
//        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, counterSSBO);
//
//        glDispatchCompute((objectCount + 63) / 64, 1, 1);
//        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
//    }
//
//    // Krok 4 — rysuj indirect
//    void DrawIndirect(GLuint VAO, GLuint EBO) {
//        glBindVertexArray(VAO);
//        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCmdSSBO);
//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, counterSSBO);
//
//        // Odczytaj liczbę widocznych
//        uint32_t visibleCount = 0;
//        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &visibleCount);
//
//        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, visibleCount, 0);
//    }
//};
//
//
//void RenderCamera(...) {
//    // === Klatka N ===
//
//    // 1. Render sceny do głównego FBO (używa Hi-Z z klatki N-1)
//    glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    culler.Cull(projection * view, objectCount); // GPU culling
//    culler.DrawIndirect(VAO, EBO);               // indirect draw
//
//    // 2. Skopiuj depth → zbuduj Hi-Z dla klatki N+1
//    culler.CopyDepth(mainDepthTex);
//    culler.BuildMips();
//
//    // 3. Wyświetl
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    // blit mainFBO → ekran
//}

////sort transparent
//for (auto& [key, vectorMesh] : instancedGroupsTransparent)
//{
//    //tylko sortuje instancjonowane obiekty
//    std::sort(vectorMesh.begin(), vectorMesh.end(),
//        [](const TransparentMesh& a, const TransparentMesh& b)
//        {
//            return a.distance > b.distance; // dalekie -> bliskie (back-to-front)
//        });
//}


//std::vector<std::pair<GroupKey, std::vector<TransparentMesh>>> items(
//    instancedGroupsTransparent.begin(),
//    instancedGroupsTransparent.end()
//);

//std::sort(items.begin(), items.end(),
//    [](auto& a, auto& b)
//    {
//        float da = a.second.front().distance;
//        float db = b.second.front().distance;
//        return da > db;
//    });




//stary render
            //Culling
              /*float dims[3] = { size.x, size.y, size.z };
                std::sort(dims, dims + 3);*/

            //shader->setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
            //shader->setVec3("dirLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
            //shader->setVec3("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
            //shader->setVec3("dirLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

            //std::vector<size_t> standardVisible;
            ////std::vector<size_t> animatedVisible;

            //for (size_t i : finalToRender) {
            //    //auto* go = renderQuery->gameobjects[i];
            //    //AnimatorComponent* animator = go->template GetComponent<AnimatorComponent>();
            //    //GameObject* current = go->GetParent();
            //    //while (animator == nullptr && current != nullptr) {
            //    //    animator = current->template GetComponent<AnimatorComponent>();
            //    //    current = current->GetParent();
            //    //}
            //   
            //    standardVisible.push_back(i);
            //}



            //if (!final.empty()) {
            //    
            //}

            //if (!animatedVisible.empty()) {

            //    for (size_t i : animatedVisible) {
            //     /*   auto* go = renderQuery->gameobjects[i];
            //        auto* renderComp = std::get<1>(renderQuery->componentsVectors)[i];*/

            //        
            //     /*   AnimatorComponent* animator = go->GetComponent<AnimatorComponent>();

            //        GameObject* current = go->GetParent();
            //        if (animator == nullptr && current != nullptr) {
            //            animator = current->GetComponentInParent<AnimatorComponent>();
            //        }*/
            //        //if (!animator && renderComp->rootAnimator) {
            //        //    animator = renderComp->rootAnimator;
            //        //}

            //        auto drawStart = std::chrono::high_resolution_clock::now();
            //        overrideMat->Apply();
            //        model->Draw(0);
            //        stats.drawCalls++;
            //        stats.renderedObjects++;
            //        stats.stateChanges++;
            //        stats.triangles += GetTriangleCount(model);
            //        auto drawEnd = std::chrono::high_resolution_clock::now();
            //        stats.drawSubmitTimeMs += std::chrono::duration<float, std::milli>(drawEnd - drawStart).count();
            //    }
            //}

