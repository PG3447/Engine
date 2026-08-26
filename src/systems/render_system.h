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
#include "GPUdriven_manager.h"



struct PerCameraHiZ
{
    GLuint hizTexture = 0;
    GLuint depthPrev = 0;
    int    hizMipLevels = 0;
    int    width = 0;
    int    height = 0;

    void Init(int w, int h, int screenW, int screenH)
    {
        width = w;
        height = h;
        hizMipLevels = static_cast<int>(std::floor(std::log2(std::max(w, h)))) + 1;

        // HiZ — R32F z mipami, taki sam format jak w GPUDrivenManager::InitHiZ
        glGenTextures(1, &hizTexture);
        glBindTexture(GL_TEXTURE_2D, hizTexture);
        glTexStorage2D(GL_TEXTURE_2D, hizMipLevels, GL_R32F, w, h);
        glTextureParameteri(hizTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTextureParameteri(hizTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(hizTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(hizTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // depth-prev — kopia depth poprzedniej klatki tej kamery
        glGenTextures(1, &depthPrev);
        glBindTexture(GL_TEXTURE_2D, depthPrev);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, screenW, screenH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTextureParameteri(depthPrev, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depthPrev, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(depthPrev, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(depthPrev, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        spdlog::info("PerCameraHiZ::Init {}x{} mips={}", w, h, hizMipLevels);
    }

    // Wywoływane tylko przy resize okna — glTexStorage2D jest immutable
    void Destroy()
    {
        if (hizTexture) { glDeleteTextures(1, &hizTexture); hizTexture = 0; }
        if (depthPrev) { glDeleteTextures(1, &depthPrev);  depthPrev = 0; }
        width = height = hizMipLevels = 0;
    }

    bool IsValid() const { return hizTexture != 0 && depthPrev != 0; }
};


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
    ECS& rendECS;
    GLFWwindow* window = nullptr;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    GLuint texture;

    SkyboxRenderer skybox;

    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 currentCameraPos;


    GLuint sceneFBO = 0;
    GLuint sceneColorTexture;

public:
    GPUDrivenManager drivenManager;

    float ambientStrength = 0.003f;// domyslnie - 0.03f;
    //GLuint sceneDepthRBO = 0;
    GLuint sceneDepthTexture;
    GLuint depthTexturePrev = 0;
    int fboWidth = 0, fboHeight = 0;
    
    std::unordered_map<CameraComponent*, PerCameraHiZ> cameraHiZ;

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

    RenderSystem(ECS& ecs, GLFWwindow* win) : rendECS(ecs), window(win)
    {
        renderQuery = ecs.CreateQuery<TransformComponent, RenderComponent>();
        lightQuery = ecs.CreateQuery<TransformComponent, LightComponent>();
        cameraQuery = ecs.CreateQuery<TransformComponent, CameraComponent>();
        animatorQuery = ecs.CreateQuery<TransformComponent, RenderComponent, AnimatorComponent>();

        drivenManager = GPUDrivenManager();

        Init();
        DebugDrawSystem::Init();
    }

    void InformedActiveECS(ECS& ecs, GLFWwindow* win) override
    {
        if (&rendECS == &ecs)
        {
            InitOpenGL();
            window = win;
        }
    }

    void Init() {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        drivenManager.Init(display_w, display_h);

        GLenum err5 = glGetError();
        if (err5 != GL_NO_ERROR)
        {
            printf("OpenGL error5: 0x%X\n", err5);
        }
        //glEnable(GL_DEPTH_TEST);
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //skybox.Init();
        //drivenManager.AddSkyboxPass(&skybox);

        //glGenQueries(2, gpuQuery.queries);

        //for (int i = 0; i < 2; i++) {
        //    glBeginQuery(GL_TIME_ELAPSED, gpuQuery.queries[i]);
        //    glEndQuery(GL_TIME_ELAPSED);
        //}

        //GLuint available = 0;
        //while (!available) {
        //    glGetQueryObjectuiv(gpuQuery.queries[0], GL_QUERY_RESULT_AVAILABLE, &available);
        //}
    }

    void InitOpenGL()
    {
        //int display_w, display_h;
        //glfwGetFramebufferSize(window, &display_w, &display_h);
        //drivenManager.InitSceneOpengl(display_w, display_h);

        //glEnable(GL_DEPTH_TEST);
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glGenQueries(2, gpuQuery.queries);

        //for (int i = 0; i < 2; i++) {
        //    glBeginQuery(GL_TIME_ELAPSED, gpuQuery.queries[i]);
        //    glEndQuery(GL_TIME_ELAPSED);
        //}

        //GLuint available = 0;
        //while (!available) {
        //    glGetQueryObjectuiv(gpuQuery.queries[0], GL_QUERY_RESULT_AVAILABLE, &available);
        //}
    }

    std::vector<RenderComponent*> pendingRegistration;

    void OnGameObjectUpdated(GameObject* e) override {
        int resultAddRender = renderQuery->OnGameObjectUpdated(e);
        if (resultAddRender <= 1) // && gpuRendererInitialized
        {
            pendingRegistration.push_back(e->GetComponent<RenderComponent>());  //drivenManager.RebuildAllRegistries(*renderQuery);
            groupsDirty = true;
            rebuildCollectData = true;
        }
        if (resultAddRender == 2)
        {
            //komponent zostal usuniety
            drivenManager.RebuildInstance();
            rebuildCollectData = true;
        }
        lightQuery->OnGameObjectUpdated(e);  // forward do query
        cameraQuery->OnGameObjectUpdated(e); // forward do query
        animatorQuery->OnGameObjectUpdated(e); 
        
    }

    void MarkDirty()
    {
        groupsDirty = true;
    }


    void Update(ECS& ecs, float dt) override {
        GLenum err8 = glGetError();
        if (err8 != GL_NO_ERROR)
        {
            printf("OpenGL error8: 0x%X\n", err8);
        }
        stats.Reset();
        GLenum err9 = glGetError();
        if (err9 != GL_NO_ERROR)
        {
            printf("OpenGL error9: 0x%X\n", err9);
        }
        gpuQuery.begin();
        GLenum err7 = glGetError();
        if (err7 != GL_NO_ERROR)
        {
            printf("OpenGL error7: 0x%X\n", err7);
        }
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        GLenum err6 = glGetError();
        if (err6 != GL_NO_ERROR)
        {
            printf("OpenGL error6: 0x%X\n", err6);
        }
        InitFBO(display_w, display_h);

        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
       /* if (occlusionCullingEnabled)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTexture, 0);
        }
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);*/

        if (groupsDirty)
        {
            for (RenderComponent* e : pendingRegistration)
                drivenManager.AddGameObjectToRegistries(e, false);
            drivenManager.FlushDirtyPasses();
            pendingRegistration.clear();
            groupsDirty = false;
        }

        //BuildGroups();
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
    //bool gpuRendererInitialized = false;
    bool rebuildCollectData = true;

    void RenderAllCameras() {
        auto& transforms = std::get<0>(cameraQuery->componentsVectors);
        auto& cameras = std::get<1>(cameraQuery->componentsVectors);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        //if (!gpuRendererInitialized) { // && renderQuery->gameobjects.size() > 50
        //    //InitGPUDrivenRenderer(display_w, display_h);
        //    //drivenManager.InitPassesFromScene(*renderQuery);
        //    gpuRendererInitialized = true;
        //    gpuRendererReady = true;
        //}
        auto& lightTransforms = std::get<0>(lightQuery->componentsVectors);
        auto& lights = std::get<1>(lightQuery->componentsVectors);

        drivenManager.UpdateAndUploadLights(lights, lightTransforms);
        drivenManager.CollectAllPasses(*renderQuery, rebuildCollectData);
        if (rebuildCollectData)
            rebuildCollectData = false;
        //drivenManager.RenderShadow();
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        //if (gpuRendererReady) {
        //    
        //}
        for (size_t i = 0; i < cameras.size(); i++) {
            if (!cameras[i]->isActive)
                continue;
            RenderCameraGPUDriven(*cameras[i], *transforms[i], display_w, display_h);
            //RenderCamera(*cameras[i], *transforms[i], display_w, display_h);
        }
    }

    //GPUDrivenRenderer gpuRenderer;
    bool gpuRendererReady = false;



    void RenderCameraGPUDriven(CameraComponent& cam, TransformComponent& transform, int width, int height)
    {
        ApplyViewport(cam.viewport, width, height);

        view = CameraHelper::getViewMatrix(cam, transform);
        projection = CameraHelper::getProjectionMatrix(cam, width, height);

        glm::mat4 vp = projection * view;
        currentCameraPos = TransformHelper::getGlobalPosition(transform);
        
        auto cullStart = std::chrono::high_resolution_clock::now();

        //drivenManager.CollectAllPasses(*renderQuery, currentCameraPos);
        drivenManager.UploadPerCamera(currentCameraPos);
        
        int vpW = std::max(1, (int)(cam.viewport.width * width));
        int vpH = std::max(1, (int)(cam.viewport.height * height));
        int vpX = (int)(cam.viewport.x * width);
        int vpY = (int)(cam.viewport.y * height);

        PerCameraHiZ& hiz = cameraHiZ[&cam];
        if (hiz.width != vpW || hiz.height != vpH)
        {
            //hiz.Destroy(); // tylko przy resize — nie co klatkę
            //hiz.Init(vpW, vpH, width, height);
        }

        drivenManager.AttachCameraHiZ(hiz.hizTexture, hiz.hizMipLevels, vpW, vpH, frustumCullingEnabled, occlusionCullingEnabled, vpX, vpY);
        drivenManager.RenderFrame(view, projection, vp, currentCameraPos, ambientStrength, occlusionCullingEnabled ? hiz.depthPrev : 0, cam.dirty);
        cam.dirty = false;
        //drivenManager.RenderFrame(vp, currentCameraPos, depthTexturePrev);
        if (hiz.depthPrev && sceneDepthTexture && occlusionCullingEnabled) {
            std::swap(sceneDepthTexture, hiz.depthPrev);
        }
        //spdlog::info("CopyDepth cam vpX={} vpY={} vpW={} vpH={} fbo={}x{}",
        //    vpX, vpY, vpW, vpH, width, height);
        //if (hiz.depthPrev && sceneDepthTexture && occlusionCullingEnabled) {
        //    glCopyImageSubData(sceneDepthTexture, GL_TEXTURE_2D, 0, vpX, vpY, 0,
        //        hiz.depthPrev, GL_TEXTURE_2D, 0, 0, 0, 0,
        //        vpW, vpH, 1);
        //}


        auto cullEnd = std::chrono::high_resolution_clock::now();
        stats.cullingTimeMs += std::chrono::duration<float, std::milli>(cullEnd - cullStart).count();

        DebugDrawSystem::Flush(vp);

        glBindVertexArray(0);
        
        //skybox.Render(view, projection);
    }


    void ShowDepthTextureImGui(GLuint depthTex, int w, int h, float zNear, float zFar)
    {
        // klucz = oryginalne ID tekstury depth
        static std::unordered_map<GLuint, GLuint> debugTexMap;

        GLuint& debugTex = debugTexMap[depthTex];
        if (debugTex == 0) {
            glGenTextures(1, &debugTex);
            glBindTexture(GL_TEXTURE_2D, debugTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        std::vector<float> depth(w * h);
        glGetTextureImage(depthTex, 0, GL_DEPTH_COMPONENT, GL_FLOAT, w * h * sizeof(float), depth.data());

        std::vector<uint8_t> rgb(w * h * 3);
        for (int i = 0; i < w * h; i++) {
            float d = depth[i];
            float linear = (2.0f * zNear) / (zFar + zNear - d * (zFar - zNear));
            uint8_t v = (uint8_t)(linear * 255.0f);
            rgb[i * 3 + 0] = v;
            rgb[i * 3 + 1] = v;
            rgb[i * 3 + 2] = v;
        }

        glBindTexture(GL_TEXTURE_2D, debugTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

        ImGui::Image((ImTextureID)(intptr_t)debugTex, ImVec2(320, 180), ImVec2(0, 1), ImVec2(1, 0));
    }
    void ShowR32FTextureImGui(GLuint tex, int mip = 0)
    {
        static std::unordered_map<GLuint, GLuint> debugTexMap;
        GLuint& debugTex = debugTexMap[tex];
        if (debugTex == 0) {
            glGenTextures(1, &debugTex);
            glBindTexture(GL_TEXTURE_2D, debugTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // Pytaj GPU o rzeczywisty rozmiar mipa
        GLint mipW = 0, mipH = 0;
        glGetTextureLevelParameteriv(tex, mip, GL_TEXTURE_WIDTH, &mipW);
        glGetTextureLevelParameteriv(tex, mip, GL_TEXTURE_HEIGHT, &mipH);
        if (mipW == 0 || mipH == 0) return;

        int pixelCount = mipW * mipH;
        size_t bufSize = std::max(pixelCount, 64);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        std::vector<float> data(bufSize, 0.0f);
        glGetTextureImage(tex, mip, GL_RED, GL_FLOAT,
            (GLsizei)(bufSize * sizeof(float)), data.data());
        glPixelStorei(GL_PACK_ALIGNMENT, 4);

        float minV = FLT_MAX, maxV = -FLT_MAX;
        for (int i = 0; i < pixelCount; i++) {
            if (data[i] > 0.0f) {
                minV = std::min(minV, data[i]);
                maxV = std::max(maxV, data[i]);
            }
        }
        if (minV >= maxV) minV = 0.0f;

        // RGBA — brak problemów z row alignment
        std::vector<uint8_t> rgba(pixelCount * 4);
        for (int i = 0; i < pixelCount; i++) {
            float   n = (maxV > minV) ? (data[i] - minV) / (maxV - minV) : 0.0f;
            uint8_t v = (uint8_t)(glm::clamp(n, 0.0f, 1.0f) * 255.0f);
            rgba[i * 4 + 0] = v;
            rgba[i * 4 + 1] = v;
            rgba[i * 4 + 2] = v;
            rgba[i * 4 + 3] = 255;
        }

        glBindTexture(GL_TEXTURE_2D, debugTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mipW, mipH, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        // Skaluj podgląd do 320px szerokości zachowując proporcje
        float dispW = 320.0f;
        float dispH = dispW * ((float)mipH / (float)mipW);
        ImGui::Text("mip%d: %dx%d", mip, mipW, mipH);
        ImGui::Image((ImTextureID)(intptr_t)debugTex,
            ImVec2(dispW, dispH), ImVec2(0, 1), ImVec2(1, 0));
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


    void InitFBO(int w, int h, bool openGL = false) {
        if (fboWidth == w && fboHeight == h && !openGL) return; // bez zmian
        fboWidth = w; fboHeight = h;

        GLenum err3 = glGetError();
        if (err3 != GL_NO_ERROR)
        {
            printf("OpenGL error3: 0x%X\n", err3);
        }
        spdlog::warn("FBO sie ustawia");
        if (sceneFBO) {
            glDeleteFramebuffers(1, &sceneFBO);
            glDeleteTextures(1, &sceneColorTexture);
            glDeleteTextures(1, &sceneDepthTexture);

            //if (depthTexturePrev) {
            //    glDeleteTextures(1, &depthTexturePrev);
            //    depthTexturePrev = 0;
            //}

            for (auto& [cam, hiz] : cameraHiZ)
                hiz.Destroy();
            cameraHiZ.clear();
        }
        GLenum err4 = glGetError();
        if (err4 != GL_NO_ERROR)
        {
            printf("OpenGL error4: 0x%X\n", err4);
        }

        glGenFramebuffers(1, &sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        GLenum err2 = glGetError();
        if (err2 != GL_NO_ERROR)
        {
            printf("OpenGL error2: 0x%X\n", err2);
        }

        glGenTextures(1, &sceneColorTexture);
        glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTexture, 0);

        glGenTextures(1, &sceneDepthTexture);
        glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            spdlog::error("SceneFBO incomplete!");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, 0);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
        {
            printf("OpenGL error: 0x%X\n", err);
        }
    }


    GLuint GetSceneTexture() const { return sceneColorTexture; }

  
    GLuint GetFirstCameraDepthPrev() const
    {
        if (cameraHiZ.empty()) return 0;
        return cameraHiZ.begin()->second.depthPrev;
    }
};

#endif