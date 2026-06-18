#ifndef GPUDRIVEN_MANAGER_H
#define GPUDRIVEN_MANAGER_H

#include <glm/glm.hpp>

#include "mesh_data.h"
#include "model.h"
#include "../core/query.h"
#include "../utils/transform_helper.h"
#include "shader.h"
#include "GPUdriven_renderer.h"

enum class RenderPassType {
    Opaque,
    Skybox,
    Transparent,
};

struct RenderPassConfig {
    RenderPassType  type;
    int sortOrder;      // kolejność wykonania
    bool depthWrite;
    bool blendingEnabled;
    Shader* shader = nullptr;  // może być nullptr
};

struct TransparentEntry {
    RenderData data;
    float      distanceSq;
};


struct PassEntry {
    uint32_t passID;
    RenderPassConfig config;
    std::unique_ptr<GPUDrivenRenderer> renderer;
    SkyboxRenderer* skyboxRenderer = nullptr;
    std::vector<RenderData> objects;
    std::vector<TransparentEntry> transparentBuffer;
};

struct FrameUBO {
    glm::mat4 viewProjection;
    glm::vec4 viewPos;   // xyz=kamera, w=unused
    float ambientStrength;
    int numLights;
    int numShadowLigths;
    int padding;
};

struct ShadowMapArray {
    GLuint fboShadow = 0;
    GLuint depthArray = 0;
    int resolution = 1024;
    int maxLayers = -1;

    void Init(int res, int layers) {
        resolution = res;
        maxLayers = layers;

        // tekstura
        glGenTextures(1, &depthArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthArray);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, res, res, layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
        //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        // FBO (bez attachmentu — ustawiamy per warstwa)
        glGenFramebuffers(1, &fboShadow);
        glBindFramebuffer(GL_FRAMEBUFFER, fboShadow);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        spdlog::critical("Inicjalizacja");
    }

    void Destroy() {
        glDeleteTextures(1, &depthArray);
        glDeleteFramebuffers(1, &fboShadow);
    }
};


class GPUDrivenManager {
public:
    int      screenWidth = 0;
    int      screenHeight = 0;
    uint32_t nextPassID = 0;
    uint32_t nextShaderID = 0;
    bool     dirtyLights = false;


    // ── Shared UBO ────────────────────────────
    GLuint frameUBO = 0;   // binding 0 — viewProj, viewPos, zNear/Far, numLights
    GLuint lightsUBO = 0;   // binding 1 — MAX_UBO_LIGHTS świateł

    GLuint hizTexture = 0;
    int    hizMipLevels = 0;

    // --- rejestry ---
    //std::unordered_map<MeshData*, uint32_t> meshRegistry;
    //std::unordered_map<Material*, uint32_t> materialRegistry;

    // --- passy (posortowane po sortOrder) ---
    std::vector<GPULight> gpuLights;
    std::vector<PassEntry> passes;
    //std::vector<std::vector<std::vector<int32_t>>> collectRenderSlots; // [entityIdx][meshIdx][passID] 
    std::vector<int32_t> collectRenderSlots;
    uint32_t collectMeshSlotCount = 0;
    uint32_t collectPassCount = 0;

    std::unordered_map<Shader*, uint32_t> opaquePassByShader;
    std::unordered_map<Shader*, uint32_t> transparentPassByShader;

    ComputeShader* shaderCountInstance = nullptr;
    ComputeShader* shaderPrefixSum = nullptr;
    ComputeShader* shaderHizWritePass = nullptr;
    ComputeShader* shaderBuildCmds = nullptr;
    ComputeShader* shaderHizDownsample = nullptr;
    Shader* defaultShaderRender = nullptr;
    Shader* depthShadowShader = nullptr;

    std::unordered_map<AnimatorComponent*, uint32_t> animatorIDMap;
    std::vector<glm::mat4> boneMatricesCache;

    ShadowMapArray shadowMapArray;

    void Init(int w, int h)
    {
        screenWidth = w;
        screenHeight = h;

        shaderCountInstance = new ComputeShader("res/shaders/hiz_culling_count.comp");
        shaderPrefixSum = new ComputeShader("res/shaders/prefix_sum.comp");
        shaderHizWritePass = new ComputeShader("res/shaders/write_pass.comp");
        shaderBuildCmds = new ComputeShader("res/shaders/build_commands.comp");
        shaderHizDownsample = new ComputeShader("res/shaders/hiz_build.comp");
        defaultShaderRender = new Shader("res/shaders/gpu_driven_PBR.vert", "res/shaders/gpu_driven_PBR.frag");
        depthShadowShader = new Shader("res/shaders/shadowDepth.vert", "res/shaders/shadowDepth.frag");

        glGenBuffers(1, &frameUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, frameUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameUBO), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, frameUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glGenBuffers(1, &lightsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightsUBO), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, lightsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // HiZ
        InitHiZ(w, h);

        spdlog::info("RendererManager::Init {}x{}", w, h);
    }

    void InitHiZ(int w, int h)
    {
        hizMipLevels = static_cast<int>(std::floor(std::log2(std::max(w, h)))) + 1;

        glGenTextures(1, &hizTexture);
        glBindTexture(GL_TEXTURE_2D, hizTexture);
        glTexStorage2D(GL_TEXTURE_2D, hizMipLevels, GL_R32F, w, h);
        glTextureParameteri(hizTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTextureParameteri(hizTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(hizTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(hizTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        spdlog::info("RendererManager: HiZ {}x{} mips={}", w, h, hizMipLevels);
    }

    void AttachCameraHiZ(GLuint hizTexture, int hizMipLevels, int vpW, int vpH, bool frustumEnabled, bool occlusionEnabled, int vpX = 0, int vpY = 0)
    {
        for (auto& entry : passes)
        {
            if (!entry.renderer) continue;
            entry.renderer->AttachHiZ(hizTexture, hizMipLevels, vpW, vpH, vpX, vpY, frustumEnabled, occlusionEnabled);  // ← vpX, vpY
        }
    }

    void InitPassesFromScene(Query<TransformComponent, RenderComponent>& renderQuery)
    {
        auto& renderers = std::get<1>(renderQuery.componentsVectors);

        for (size_t i = 0; i < renderers.size(); i++) {
            RenderComponent* rc = renderers[i];
            if (!rc) continue;

            if (rc->animator && animatorIDMap.find(rc->animator) == animatorIDMap.end())
                animatorIDMap[rc->animator] = (uint32_t)animatorIDMap.size();

            for (auto& mesh : rc->meshes) {
                if (!mesh.cpuData || !mesh.material) continue;

                Material* mat = mesh.material.get();
                Shader* shader = mat->shader ? mat->shader : defaultShaderRender;

                // Pobierz lub stwórz pass dla tego (shader, surfaceType)
                uint32_t pid = GetOrCreatePass(shader, mat->surfaceType);

                GPUDrivenRenderer* r = GetRenderer(pid);
                if (!r) continue;

                r->RegisterMesh(pid, rc, mesh.cpuData.get());
                r->RegisterMaterial(pid, rc, mat);
            }
        }

        // Wyślij geometrię i materiały każdego pasa na GPU
        for (auto& entry : passes) {
            if (!entry.renderer) continue;

            entry.renderer->UploadMeshes();
            entry.renderer->UploadMaterials();
        }

        spdlog::info("RendererManager: zainicjalizowano {} passów", passes.size());
    }


    //void RebuildAllRegistries(Query<TransformComponent, RenderComponent>& renderQuery) {
    //    // 1. Wyczyść wszystkie passy
    //    for (auto& entry : passes) {
    //        GPUDrivenRenderer* r = entry.renderer.get();

    //        // Wyczyść CPU-side rejestry i bufory geometrii
    //        r->clearRegisterAll();
    //    }

    //    animatorIDMap.clear();

    //    // 2. Zarejestruj wszystko od nowa (jak InitPassesFromScene)
    //    // ale NIE czyść samych passów — zostają te same passy
    //    auto& renderers = std::get<1>(renderQuery.componentsVectors);

    //    for (size_t i = 0; i < renderers.size(); i++) {
    //        RenderComponent* rc = renderers[i];
    //        if (!rc) continue;

    //        if (rc->animator && animatorIDMap.find(rc->animator) == animatorIDMap.end())
    //            animatorIDMap[rc->animator] = (uint32_t)animatorIDMap.size();

    //        for (auto& mesh : rc->meshes) {
    //            if (!mesh.cpuData || !mesh.material) continue;

    //            Material* mat = mesh.material.get();
    //            Shader* shader = mat->shader ? mat->shader : defaultShaderRender;

    //            uint32_t pid = GetOrCreatePass(shader, mat->surfaceType);
    //            GPUDrivenRenderer* r = GetRenderer(pid);
    //            if (!r) continue;

    //            r->RegisterMesh(mesh.cpuData.get());
    //            r->RegisterMaterial(mat);
    //        }
    //    }

    //    // 3. Upload na GPU
    //    for (auto& entry : passes) {
    //        entry.renderer->UploadMeshes();
    //        entry.renderer->UploadMaterials();
    //        entry.renderer->dirtyInstance = true;
    //    }
    //}
    std::unordered_map<uint32_t, bool> meshDirty;

    void RebuildAllRegistries(Query<TransformComponent, RenderComponent>& renderQuery)
    {
        auto& renderers = std::get<1>(renderQuery.componentsVectors);

        for (size_t i = 0; i < renderers.size(); i++) {
            RenderComponent* rc = renderers[i];
            if (!rc) continue;

            if (rc->animator && animatorIDMap.find(rc->animator) == animatorIDMap.end())
                animatorIDMap[rc->animator] = (uint32_t)animatorIDMap.size();

            for (auto& mesh : rc->meshes) {
                if (!mesh.cpuData || !mesh.material) continue;

                Material* mat = mesh.material.get();
                Shader* shader = mat->shader ? mat->shader : defaultShaderRender;
                uint32_t  pid = GetOrCreatePass(shader, mat->surfaceType);

                GPUDrivenRenderer* r = GetRenderer(pid);
                if (!r) continue;

                // RegisterMesh / RegisterMaterial są idempotentne —
                // zwracają istniejące ID gdy zasób już jest zarejestrowany.
                // Sprawdzamy przed wywołaniem czy coś faktycznie jest nowe,
                // żeby wiedzieć czy należy re-uploadować.
                //bool meshIsNew = (r->GetMeshId(mesh.cpuData.get()) == UINT32_MAX);
                //bool materialIsNew = (r->GetMaterialId(mat) == UINT32_MAX);
                bool meshIsNew = (mesh.cpuData->getMeshId(pid) == UINT32_MAX); // (r->GetMeshId(mesh.cpuData.get()) == UINT32_MAX);
                bool materialIsNew = (mat->getMaterialId(pid) == UINT32_MAX); // (r->GetMaterialId(mat) == UINT32_MAX);

                if (meshIsNew)     r->RegisterMesh(pid, rc, mesh.cpuData.get());
                if (materialIsNew) r->RegisterMaterial(pid, rc, mat);

                if (meshIsNew || materialIsNew)
                    meshDirty[pid] = true;
            }

            // Każda zmiana obiektu wymaga odbudowy instancji
            // (transform mógł się zmienić, obiekt mógł być dodany/usunięty)
            for (auto& entry : passes)
            {
                if (!entry.renderer) continue;
                entry.renderer->dirtyInstance = true;
            }
        }

        // Flush tylko passów które faktycznie dostały nowe zasoby
        for (auto& entry : passes) {
            if (!entry.renderer) continue;
            auto it = meshDirty.find(entry.passID);
            if (it == meshDirty.end() || !it->second) continue;

            entry.renderer->UploadMeshes();
            entry.renderer->UploadMaterials();
            it->second = false;
            spdlog::info("RendererManager: flush pass {}", entry.passID);
        }
    }

    void RebuildInstance()
    {
        for (auto& entry : passes)
        {
            if (!entry.renderer) continue;
            entry.renderer->dirtyInstance = true;
        }
    }

    void AddGameObjectToRegistries(RenderComponent* rc)
    {
        if (!rc) return;

        //if (rc->animator && animatorIDMap.find(rc->animator) == animatorIDMap.end())
        //    animatorIDMap[rc->animator] = (uint32_t)animatorIDMap.size();

        for (auto& mesh : rc->meshes) {
            if (!mesh.cpuData || !mesh.material) continue;
            Material* mat = mesh.material.get();
            Shader* shader = mat->shader ? mat->shader : defaultShaderRender;
            uint32_t pid = GetOrCreatePass(shader, mat->surfaceType);

            GPUDrivenRenderer* r = GetRenderer(pid);
            if (!r) continue;
            
            //bool meshIsNew = (r->GetMeshId(mesh.cpuData.get()) == UINT32_MAX);
            //bool materialIsNew = (r->GetMaterialId(mat) == UINT32_MAX);
            bool meshIsNew = (mesh.cpuData->getMeshId(pid) == UINT32_MAX); //(r->GetMeshId(mesh.cpuData.get()) == UINT32_MAX);
            bool materialIsNew = (mat->getMaterialId(pid) == UINT32_MAX);// (r->GetMaterialId(mat)== UINT32_MAX);

            if (meshIsNew)     r->RegisterMesh(pid, rc, mesh.cpuData.get());
            if (materialIsNew) r->RegisterMaterial(pid, rc, mat);

            if (meshIsNew || materialIsNew)
                meshDirty[pid] = true;
        }

        // Flush tylko passów które dostały nowe zasoby
        for (auto& entry : passes) {
            if (!entry.renderer) continue;
            entry.renderer->dirtyInstance = true;
            auto it = meshDirty.find(entry.passID);
            if (it == meshDirty.end() || !it->second) continue;

            entry.renderer->UploadMeshes();
            entry.renderer->UploadMaterials();
            it->second = false;
            spdlog::info("RendererManager: flush pass {}", entry.passID);
        }
    }



    void CollectRenderData(uint32_t passID, Query<TransformComponent, RenderComponent>& renderQuery, bool rebuildCollectData) //, const glm::vec3& cameraPos
    {
        PassEntry* entry = FindPass(passID);
        if (!entry) return;
        if (!entry->renderer.get()) return;

        //GPUDrivenRenderer* r = entry->renderer.get();
        const SurfaceType filter = PassTypeToSurface(entry->config.type);
        const bool isTransparent = (entry->config.type == RenderPassType::Transparent);
        Shader* passShader = entry->config.shader ? entry->config.shader : defaultShaderRender;

        auto& transforms = std::get<0>(renderQuery.componentsVectors);
        auto& renderers = std::get<1>(renderQuery.componentsVectors);
        const size_t count = renderQuery.gameobjects.size();

        // ── 4. Buduj RenderData ───────────────────────────────────
        if (rebuildCollectData) {
            entry->objects.clear();
            entry->objects.reserve(count);

            //collectRenderSlots.resize(count);
            //for (size_t i = 0; i < count; ++i) {
            //    RenderComponent* rc = renderers[i];
            //    if (!rc) continue;
            //    // Rozszerz do liczby meshów tej encji
            //    collectRenderSlots[i].resize(MeshNode::GetNextID(), std::vector<int32_t>(passes.size(), -1));
            //}
            //collectRenderSlots.resize(count, std::vector<int32_t>(std::vector<int32_t>(passes.size(), -1)))
            //(MeshNode::GetNextID(), std::vector<int32_t>(passes.size(), -1));
        }

        entry->transparentBuffer.clear();
        for (size_t i = 0; i < count; ++i) {
            const TransformComponent* t = transforms[i];
            RenderComponent* rc = renderers[i];
            if (!t || !rc) continue;


            if (!rc->rendererDirty && !t->rendererDirty && !rebuildCollectData && !isTransparent)
            {
                //spdlog::error("OMIJAM");
                continue;
                
                //spdlog::error("TRANSFORM NIE GIT");
            } //&& animIt == NO_SKELETON


            const glm::mat4 model = t->modelMatrix;

            auto animIt = rc->animator ? rc->animator->animatorID : NO_SKELETON;
            //auto animIt = rc->animator ? animatorIDMap.find(rc->animator) : animatorIDMap.end();

            //rc->animator->animatorID;

            for (auto& mesh : rc->meshes) {
                if (!mesh.cpuData || !mesh.material) continue;

                Material* mat = mesh.material.get();
                Shader* shader = mat->shader ? mat->shader : defaultShaderRender;

                // Filtruj: ten pass obsługuje tylko swój shader i swój surfaceType
                if (shader != passShader || mat->surfaceType != filter) continue;

                uint32_t meshID = mesh.cpuData->getMeshId(passID);// r->GetMeshId(mesh.cpuData.get());// mesh.cpuData->meshID;// r->GetMeshId(mesh.cpuData.get());
                uint32_t matID = mat->getMaterialId(passID); //r->GetMaterialId(mat);  //mat->materialID;// r->GetMaterialId(mat);
                if (meshID == UINT32_MAX || matID == UINT32_MAX) continue;

                const auto& aabb = mesh.cpuData->aabb;

                RenderData rd{
                    .modelMatrix = model,
                    .aabbMin = glm::vec4(aabb.min, 0.0f),
                    .aabbMax = glm::vec4(aabb.max, 0.0f),
                    .meshID = meshID,
                    .materialID = matID,
                    .skeletonID = animIt, // (animIt != animatorIDMap.end() ? animIt->second : NO_SKELETON),
                    .padding = 0
                };


                if (isTransparent) {
                    //glm::vec3 worldCenter = glm::vec3(model * glm::vec4(aabb.centerLocal, 1.0f));
                    //float distSq = glm::length2(cameraPos - worldCenter);
                    //transparentBuffer.push_back({ rd, distSq });
                    entry->transparentBuffer.push_back({ rd, 0.0 });
                }
                else {
                    uint32_t idx = i * collectMeshSlotCount * collectPassCount + mesh.meshNodeID * collectPassCount + passID;
                    if (rebuildCollectData)
                    {
                        collectRenderSlots[idx] = (int32_t)entry->objects.size();
                        entry->objects.push_back(rd);
                    }
                    else
                    {
                        entry->objects[collectRenderSlots[idx]] = rd;
                    }
                }
            }

            
        }

        //// ── 5. Transparent: sort back-to-front ────────────────────
        //if (isTransparent && !transparentBuffer.empty()) {
        //    std::sort(transparentBuffer.begin(), transparentBuffer.end(),
        //        [](const TransparentEntry& a, const TransparentEntry& b) {
        //            return a.distanceSq > b.distanceSq;
        //        });
        //    entry->objects.reserve(transparentBuffer.size());
        //    for (auto& te : transparentBuffer)
        //        entry->objects.push_back(te.data);
        //}
    }


    /*
                    if (matID == -1)
                {
                    matID = r->RegisterMaterial(rc, mat);
                    r->UploadMaterials();
                }
    */

    void UpdateBoneCache(Query<TransformComponent, RenderComponent>& renderQuery)
    {
        auto& transforms = std::get<0>(renderQuery.componentsVectors);
        auto& renderers = std::get<1>(renderQuery.componentsVectors);
        const size_t count = renderQuery.gameobjects.size();

        //for (size_t i = 0; i < count; ++i) {
        //    const RenderComponent* rc = renderers[i];
        //    if (!rc || !rc->animator) continue;
        //    if (animatorIDMap.find(rc->animator) == animatorIDMap.end())
        //        animatorIDMap[rc->animator] = (uint32_t)animatorIDMap.size();
        //}

        const size_t requiredBones = staticCounterAnimator * MAX_BONES_PER_SKELETON;// animatorIDMap.size()* MAX_BONES_PER_SKELETON;
        if (boneMatricesCache.size() != requiredBones)
            boneMatricesCache.resize(requiredBones, glm::mat4(1.0f));

        for (size_t i = 0; i < count; ++i) {
            TransformComponent* t = transforms[i];
            RenderComponent* rc = renderers[i];
            t->rendererDirty = false;
            rc->rendererDirty = false;
            if (!rc || !rc->animator || !rc->animator->currentSkeleton) continue;

            //auto animIt = animatorIDMap.find(rc->animator);
            //if (animIt == animatorIDMap.end()) continue;

            //const uint32_t slot = rc->animator->animatorID; //animIt->second;
            const uint32_t boneCount = (uint32_t)std::min(rc->animator->finalBoneMatrices.size(), (size_t)MAX_BONES_PER_SKELETON);

            std::memcpy(boneMatricesCache.data() + rc->animator->animatorID * MAX_BONES_PER_SKELETON, rc->animator->finalBoneMatrices.data(), boneCount * sizeof(glm::mat4));
        }
    }

    void CollectAllPasses(Query<TransformComponent, RenderComponent>& renderQuery, bool rebuildCollectData) //,  const glm::vec3& cameraPos
    {
        if (rebuildCollectData)
        {
            collectMeshSlotCount = MeshNode::GetNextID();
            collectPassCount = (uint32_t)passes.size();

            collectRenderSlots.assign(renderQuery.gameobjects.size() * collectMeshSlotCount * collectPassCount, -1);
        }

        for (auto& entry : passes)
            CollectRenderData(entry.passID, renderQuery, rebuildCollectData);// , cameraPos);

        // ── Kości raz, nie N razy per pass ───────────────────────────
        UpdateBoneCache(renderQuery);
        
        for (auto& entry : passes)
        {
            if (!entry.renderer) continue;
            entry.renderer->ResizeBoneBufferIfNeeded(staticCounterAnimator);
            entry.renderer->UploadAllBoneMatrices(boneMatricesCache);
        }
        //// ── Upload kości do każdego renderera ─────────────────────────
        //for (auto& entry : passes) {
        //    //entry.renderer->ResizeBoneBufferIfNeeded((uint32_t)animatorIDMap.size());
        //    entry.renderer->ResizeBoneBufferIfNeeded(staticCounterAnimator);
        //    entry.renderer->UploadAllBoneMatrices(boneMatricesCache);
        //}
    }

    void UploadPerCamera(const glm::vec3& cameraPos)
    {
        for (auto& entry : passes)
        {
            if (entry.config.type != RenderPassType::Transparent) continue;
            if (!entry.transparentBuffer.empty()) {

                // Najpierw przelicz dystans dla tej kamery
                for (auto& te : entry.transparentBuffer) {
                    glm::vec3 center = glm::vec3(te.data.modelMatrix * glm::vec4(0, 0, 0, 1));
                    te.distanceSq = glm::length2(cameraPos - center);
                }

                // Dopiero teraz sortuj
                std::sort(entry.transparentBuffer.begin(), entry.transparentBuffer.end(),
                    [](const TransparentEntry& a, const TransparentEntry& b) {
                        return a.distanceSq > b.distanceSq;
                    });

                // Przepisz do objects
                entry.objects.clear();
                entry.objects.reserve(entry.transparentBuffer.size());
                for (auto& te : entry.transparentBuffer)
                    entry.objects.push_back(te.data);
            }

            //entry.renderer->ResizeBoneBufferIfNeeded((uint32_t)animatorIDMap.size());
        }
    }

    //  Zarządzanie passami
       //  Każdy pass rejestruje własne mesze i materiały przez GetRenderer()
       //  lub przez dedykowane metody RegisterMeshInPass / RegisterMaterialInPass.
       // ─────────────────────────────────────────────────────────────────
    uint32_t AddPass(RenderPassConfig cfg)
    {
        uint32_t id = nextPassID++;

        PassEntry entry;
        entry.passID = id;
        entry.config = cfg;

        entry.renderer = std::make_unique<GPUDrivenRenderer>();
        entry.renderer->clearRegisterAll();
        entry.renderer->Init(screenWidth, screenHeight);
        entry.renderer->shaderHizCullCount = shaderCountInstance;
        entry.renderer->shaderPrefixSum = shaderPrefixSum;
        entry.renderer->shaderHizWritePass = shaderHizWritePass;
        entry.renderer->shaderBuildCmds = shaderBuildCmds;
        entry.renderer->shaderHizDownsample = shaderHizDownsample;
        entry.renderer->shaderRender = cfg.shader ? cfg.shader : defaultShaderRender;
        entry.renderer->shaderShadowRender = depthShadowShader;
        entry.renderer->AttachHiZ(hizTexture, hizMipLevels, screenWidth, screenHeight);
        spdlog::info("Add pass");
        passes.push_back(std::move(entry));

        std::sort(passes.begin(), passes.end(),
            [](const PassEntry& a, const PassEntry& b) {
                return a.config.sortOrder < b.config.sortOrder;
            });

        return id;
    }

    uint32_t AddSkyboxPass(SkyboxRenderer* skybox)
    {
        RenderPassConfig cfg{
            .type = RenderPassType::Skybox,
            .sortOrder = 50,
            .depthWrite = false,
            .blendingEnabled = false,
            .shader = nullptr
        };

        uint32_t id = nextPassID++;
        PassEntry entry;
        entry.passID = id;
        entry.config = cfg;
        entry.skyboxRenderer = skybox;
        entry.renderer = nullptr;

        passes.push_back(std::move(entry));
        std::sort(passes.begin(), passes.end(),
            [](const PassEntry& a, const PassEntry& b) {
                return a.config.sortOrder < b.config.sortOrder;
            });
        return id;
    }

    void RemovePass(uint32_t passID)
    {
        auto it = std::find_if(passes.begin(), passes.end(),
            [passID](const PassEntry& e) { return e.passID == passID; });
        if (it != passes.end())
            passes.erase(it);
    }

    // Dostęp do renderera konkretnego pasa —
    // używaj do RegisterMesh / RegisterMaterial / podpięcia compute shaderów
    GPUDrivenRenderer* GetRenderer(uint32_t passID)
    {
        for (auto& e : passes)
            if (e.passID == passID) return e.renderer.get();
        return nullptr;
    }

    // Wygodne wrappery: rejestracja zasobu w konkretnym pasie
    uint32_t RegisterMeshInPass(uint32_t passID, RenderComponent* rc, MeshData* d)
    {
        GPUDrivenRenderer* r = GetRenderer(passID);
        if (!r) return UINT32_MAX;
        return r->RegisterMesh(passID, rc, d);
    }

    uint32_t RegisterMaterialInPass(uint32_t passID, RenderComponent* rc, Material* m)
    {
        GPUDrivenRenderer* r = GetRenderer(passID);
        if (!r) return UINT32_MAX;
        return r->RegisterMaterial(passID, rc, m);
    }

    // Wyślij geometrię i materiały danego pasa na GPU
    // (wywołaj po zakończeniu rejestracji, przed pierwszą klatką)
    void FlushPass(uint32_t passID)
    {
        GPUDrivenRenderer* r = GetRenderer(passID);
        if (!r) return;
        r->UploadMeshes();
        r->UploadMaterials();
    }

    // ─────────────────────────────────────────────────────────────────
    //  Submisje obiektów
    // ─────────────────────────────────────────────────────────────────

    void Submit(uint32_t passID, const RenderData& obj)
    {
        for (auto& e : passes) {
            if (e.passID == passID) {
                e.objects.push_back(obj);
                e.renderer->dirtyInstance = true;
                return;
            }
        }
    }

    void ClearSubmissions()
    {
        for (auto& e : passes)
            e.objects.clear();
    }

    void UpdateAndUploadLights(std::vector<LightComponent*>& lights, std::vector<TransformComponent*>& transforms)
    {
        if (lights.empty()) return;

        uint32_t count = std::min((uint32_t)lights.size(), (uint32_t)MAX_UBO_LIGHTS);
        gpuLights.resize(count);

        std::vector<glm::mat4> lightSpaceMatrix(count);

        for (uint32_t i = 0; i < count; i++) {
            LightComponent* light = lights[i];
            TransformComponent* transform = transforms[i];
            if (!light || !transform) continue;

            GPULight& g = gpuLights[i];
            const bool on = light->isOn;
            const glm::vec3 zero(0.0f);

            g.position = glm::vec4(TransformHelper::getGlobalPosition(*transform), (float)light->type);
            g.direction = (glm::length2(light->direction) < 0.0001f) ? glm::vec4(TransformHelper::getForward(*transform), 0.0f) : glm::vec4(light->direction, 0.0f);
            g.ambient = glm::vec4(on ? light->ambient : zero, 0.0f);
            g.diffuse = glm::vec4(on ? light->diffuse : zero, 0.0f);
            g.specular = glm::vec4(on ? light->specular : zero, 0.0f);
            g.params1 = glm::vec4(light->constant, light->linear, light->quadratic, light->intensity);
            g.params2 = glm::vec4(light->cutOff, light->outerCutOff, on ? 1.0f : 0.0f, light->range);


            //spdlog::info("[Light {}] type={} pos=({:.3f}, {:.3f}, {:.3f}) dir=({:.3f}, {:.3f}, {:.3f})",
            //    i, (int)light->type,
            //    g.position.x, g.position.y, g.position.z,
            //    g.direction.x, g.direction.y, g.direction.z);

            float near_plane = 1.0f, far_plane = 500.0f;
            glm::mat4 lightProjection, lightView;
            auto safeUp = [](glm::vec3 dir) -> glm::vec3 {
                // unikamy równoległości forward/up
                dir = glm::normalize(dir);
                return (glm::abs(glm::dot(dir, glm::vec3(0, 1, 0))) > 0.99f)
                    ? glm::vec3(0, 0, 1)
                    : glm::vec3(0, 1, 0);
                };

            glm::vec3 pos = TransformHelper::getGlobalPosition(*transform);
            glm::vec3 dir = glm::normalize(glm::vec3(g.direction)); // już ustawiony wyżej
            if (light->type == Directional) {
                lightProjection = glm::ortho(-10.f, 10.f, -10.f, 10.f, near_plane, far_plane);
                lightView = glm::lookAt(pos, pos + dir, safeUp(dir));
                lightSpaceMatrix[i] = lightProjection * lightView;
            }

            if (light->type == Spot) {
                float fov = glm::acos(glm::clamp(light->outerCutOff, -1.f, 1.f)) * 2.f;
                lightProjection = glm::perspective(fov, 1.0f, near_plane, far_plane);
                //lightView = glm::lookAt(pos, pos + dir, safeUp(dir));
                lightView = glm::lookAt(pos, glm::vec3(g.position) + TransformHelper::getForward(*transform), glm::vec3(0.0f, 1.0f, 0.0f));
                lightSpaceMatrix[i] = lightProjection * lightView;
            }
        }

        glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(GPULight), gpuLights.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
        for (auto& entry : passes) {
            if (entry.config.type == RenderPassType::Skybox) {
                continue;
            }

            if (entry.renderer)
            {
                entry.renderer->UploadShadowMatrix(lightSpaceMatrix);
            }
        }
    }

    void DebugRenderFrameInput(const PassEntry& entry, const glm::vec3& cameraPos)
    {
        spdlog::info("=== PassID={} type={} objects={} transparentBuffer={} ===",
            entry.passID,
            entry.config.type == RenderPassType::Transparent ? "Transparent" : "Opaque",
            entry.objects.size(),
            entry.transparentBuffer.size()
        );

        for (size_t i = 0; i < entry.objects.size(); ++i)
        {
            const RenderData& rd = entry.objects[i];
            glm::vec3 pos = glm::vec3(rd.modelMatrix[3]);

            spdlog::info("  [{}] meshID={} matID={} skelID={} pos=({:.2f},{:.2f},{:.2f})",
                i,
                rd.meshID,
                rd.materialID,
                rd.skeletonID,
                pos.x, pos.y, pos.z
            );

            if (rd.meshID == UINT32_MAX)
                spdlog::error("    ^ meshID UINT32_MAX!");
            if (rd.materialID == UINT32_MAX)
                spdlog::error("    ^ materialID UINT32_MAX!");
            if (rd.skeletonID != NO_SKELETON)
                spdlog::info("    ^ animowany, skeletonID={}", rd.skeletonID);
        }

        if (entry.objects.empty())
            spdlog::warn("  PUSTY — nic nie idzie do GPU!");
    }

    int SHADOW_RESOLUTION = 1024;
    void RenderShadow()
    {
        const int numLights = (int)gpuLights.size();
        if (numLights == 0) return;
        if (shadowMapArray.maxLayers != numLights)
        {
            if (shadowMapArray.fboShadow != 0)
                shadowMapArray.Destroy();
            
            shadowMapArray.Init(SHADOW_RESOLUTION, numLights);
        }

        // ── zapis aktualnego stanu ──
        GLint  prevViewport[4];
        glGetIntegerv(GL_VIEWPORT, prevViewport);

        GLint  prevFBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

        GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);

        GLint  prevCullFaceMode = GL_BACK;
        glGetIntegerv(GL_CULL_FACE_MODE, &prevCullFaceMode);


        glViewport(0, 0, shadowMapArray.resolution, shadowMapArray.resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapArray.fboShadow);
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_FRONT);
        bool first = true;

        for (uint32_t i = 0; i < numLights; i++) {

            UploadFrameUBO(glm::mat4(1.0f), glm::vec3(1.0f), 0.0f, numLights, i);

            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMapArray.depthArray, 0, i);
            glClear(GL_DEPTH_BUFFER_BIT);
            

            //Render depth texture - shadow map
            for (auto& entry : passes)
            {
                if (entry.config.type == RenderPassType::Skybox || entry.config.type == RenderPassType::Transparent)
                {
                    continue;
                }

                if (entry.renderer)
                {
                    entry.renderer->RenderShadow(first, entry.objects);
                }

            }
            first = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        glCullFace(prevCullFaceMode);
        if (!prevDepthTest)
            glDisable(GL_DEPTH_TEST);
    }

    // Główna pętla renderowania
    void RenderFrame(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& viewProj, glm::vec3 cameraPos, float ambientStrength, GLuint prevDepth, bool cameraDirty, float zNear = 0.1f, float zFar = 1000.0f)
    {
        const int numLights = (int)gpuLights.size();
        UploadFrameUBO(viewProj, cameraPos, ambientStrength, numLights, numLights);

        //if (prevDepth && hizTexture) {
        //    glCopyImageSubData(prevDepth, GL_TEXTURE_2D, 0, 0, 0, 0,
        //        hizTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
        //        screenWidth, screenHeight, 1);
        //    BuildHiZ();
        //}
        //spdlog::info("Renderowanie");
        if (prevDepth != 0 && true && !passes.empty())
        {
            passes[0].renderer->BuildHiZ(prevDepth);
        }


        for (auto& entry : passes) {
            if (entry.config.type == RenderPassType::Skybox) {
                if (entry.skyboxRenderer)
                    entry.skyboxRenderer->Render(view, projection); // ← view/projection trzeba przekazać
                continue;
            }

            if (entry.objects.empty()) continue;
            ApplyPassState(entry.config);
            

            if (entry.renderer)
            {
                //DebugRenderFrameInput(entry, cameraPos);
                entry.renderer->RenderFrame(viewProj, entry.objects, prevDepth, shadowMapArray.depthArray, cameraPos, cameraDirty);
            }
        }

        // Przywróć domyślny stan po wszystkich passach
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void ApplyPassState(const RenderPassConfig& cfg)
    {
        glDepthMask(cfg.depthWrite ? GL_TRUE : GL_FALSE);
        if (cfg.blendingEnabled) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else {
            glDisable(GL_BLEND);
        }
    }

    void UploadFrameUBO(const glm::mat4& viewProj, const glm::vec3& cameraPos, float ambientStrength, int numLights, int numShadowLights)
    {
        FrameUBO data{};
        data.viewProjection = viewProj;
        data.viewPos = glm::vec4(cameraPos, 1.0f);
        data.ambientStrength = ambientStrength;
        data.numLights = numLights;
        data.numShadowLigths = numShadowLights;

        glBindBuffer(GL_UNIFORM_BUFFER, frameUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FrameUBO), &data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


    void BuildHiZ()
    {
        for (auto& entry : passes) {
            if (!entry.renderer) continue;
            if (entry.renderer->shaderHizDownsample) {
                //entry.renderer->BuildHiZ();
                return;
            }
        }
    }

    PassEntry* FindPass(uint32_t passID)
    {
        for (auto& e : passes)
            if (e.passID == passID) return &e;
        return nullptr;
    }

    // Mapuje RenderPassType na SurfaceType z Material
    static SurfaceType PassTypeToSurface(RenderPassType type)
    {
        switch (type) {
            case RenderPassType::Opaque:      return SurfaceType::Opaque;
            case RenderPassType::Transparent: return SurfaceType::Transparent;
            default:                          return SurfaceType::Opaque;
        }
    }

    int32_t GetOrCreatePass(Shader* shader, SurfaceType surface)
    {
        auto& registry = (surface == SurfaceType::Transparent) ? transparentPassByShader : opaquePassByShader;

        auto it = registry.find(shader);
        if (it != registry.end())
            return it->second;

        // Nowy pass — Opaque przed Transparent (sortOrder)
        const bool isTransparent = (surface == SurfaceType::Transparent);
        RenderPassConfig cfg{
            .type = isTransparent ? RenderPassType::Transparent : RenderPassType::Opaque,
            .sortOrder = isTransparent ? 100 : 0,
            .depthWrite = !isTransparent,
            .blendingEnabled = isTransparent,
            .shader = shader
        };

        uint32_t pid = AddPass(cfg);
        registry[shader] = pid;

        spdlog::info("RendererManager: nowy pass {} shader={} type={}",
            pid, (void*)shader,
            isTransparent ? "Transparent" : "Opaque");
        return pid;
    }


    void DebugShadowMapImGui()
    {
        // ── Shadery (inline, kompilowane raz) ────────────────────────────────────
        static const char* kVert = R"glsl(
#version 430 core
out vec2 vUV;
void main() {
    vec2 p  = vec2((gl_VertexID & 1) << 2, (gl_VertexID & 2) << 1) - 1.0;
    vUV     = p * 0.5 + 0.5;
    gl_Position = vec4(p, 0.0, 1.0);
})glsl";

        static const char* kFrag = R"glsl(
#version 430 core
in  vec2 vUV;
out vec4 fragColor;
uniform sampler2DArray uDepthArray;
uniform int   uLayer;
uniform float uNear;
uniform float uFar;
void main() {
    float d   = texture(uDepthArray, vec3(vUV, float(uLayer))).r;
    float lin = (2.0 * uNear) / (uFar + uNear - d * (uFar - uNear));
    fragColor = vec4(vec3(lin), 1.0);
})glsl";

        // ── Zasoby GL (lazy init, niszczone przy zamknięciu okna) ───────────────
        static GLuint prog = 0;
        static GLuint vao = 0;
        static GLuint tex = 0;
        static GLuint fbo = 0;
        static int    texRes = 0;

        // ── ImGui okno ──────────────────────────────────────────────────────────
        if (!ImGui::Begin("Shadow Debug")) { ImGui::End(); return; }

        const int usedLayers = (int)gpuLights.size();
        const int maxLayers = shadowMapArray.maxLayers;

        ImGui::Text("Lights: %d", usedLayers);
        ImGui::Text("Shadow layers: %d / %d", usedLayers, maxLayers);
        ImGui::Text("Resolution: %dx%d", shadowMapArray.resolution, shadowMapArray.resolution);
        ImGui::Text("FBO: %u  DepthArray: %u", shadowMapArray.fboShadow, shadowMapArray.depthArray);
        ImGui::Separator();

        static int   layer = 0;
        static float previewSize = 256.f;
        static float near_z = 1.f;
        static float far_z = 1000.f;

        if (usedLayers <= 0 || shadowMapArray.depthArray == 0)
        {
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                "Brak swiatel lub depth array nie zainicjalizowane");
            ImGui::End();
            return;
        }

        ImGui::SliderInt("Layer", &layer, 0, usedLayers - 1);
        ImGui::SliderFloat("Preview size", &previewSize, 128.f, 512.f);
        ImGui::SliderFloat("Near", &near_z, 0.01f, 10.f);
        ImGui::SliderFloat("Far", &far_z, 10.f, 2000.f);

        // ── Lazy init shadera + VAO ──────────────────────────────────────────────
        if (prog == 0)
        {
            auto compile = [](GLenum type, const char* src) -> GLuint {
                GLuint s = glCreateShader(type);
                glShaderSource(s, 1, &src, nullptr);
                glCompileShader(s);
                GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
                if (!ok) {
                    char b[512]; glGetShaderInfoLog(s, 512, nullptr, b);
                    spdlog::error("ShadowDebug shader: {}", b);
                }
                return s;
                };
            GLuint vs = compile(GL_VERTEX_SHADER, kVert);
            GLuint fs = compile(GL_FRAGMENT_SHADER, kFrag);
            prog = glCreateProgram();
            glAttachShader(prog, vs); glAttachShader(prog, fs);
            glLinkProgram(prog);
            glDeleteShader(vs); glDeleteShader(fs);
            glGenVertexArrays(1, &vao);
        }

        // ── Lazy init / przebudowa tekstury i FBO ───────────────────────────────
        const int res = shadowMapArray.resolution;
        if (tex == 0 || texRes != res)
        {
            if (tex) glDeleteTextures(1, &tex);
            if (fbo) glDeleteFramebuffers(1, &fbo);

            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, res, res, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                spdlog::error("ShadowDebug: FBO niekompletne");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            texRes = res;
        }

        // ── Zapis stanu GL ───────────────────────────────────────────────────────
        GLint prevFBO = 0;      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
        GLint prevVP[4];        glGetIntegerv(GL_VIEWPORT, prevVP);
        GLint prevProg = 0;     glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
        GLint prevVAO = 0;      glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
        GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
        GLboolean prevBlend = glIsEnabled(GL_BLEND);

        // ── Render depth array → tex ─────────────────────────────────────────────
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, res, res);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        glUseProgram(prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapArray.depthArray);
        glUniform1i(glGetUniformLocation(prog, "uDepthArray"), 0);
        glUniform1i(glGetUniformLocation(prog, "uLayer"), layer);
        glUniform1f(glGetUniformLocation(prog, "uNear"), near_z);
        glUniform1f(glGetUniformLocation(prog, "uFar"), far_z);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // ── Przywróć stan GL ─────────────────────────────────────────────────────
        glBindVertexArray(prevVAO);
        glUseProgram(prevProg);
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glViewport(prevVP[0], prevVP[1], prevVP[2], prevVP[3]);
        if (prevDepth) glEnable(GL_DEPTH_TEST);  else glDisable(GL_DEPTH_TEST);
        if (prevBlend) glEnable(GL_BLEND);       else glDisable(GL_BLEND);

        // ── Podgląd ──────────────────────────────────────────────────────────────
        ImGui::Text("Layer %d  (near=%.2f  far=%.2f)", layer, near_z, far_z);
        ImGui::Image(
            reinterpret_cast<ImTextureID>(static_cast<intptr_t>(tex)),
            ImVec2(previewSize, previewSize),
            ImVec2(0, 1), ImVec2(1, 0));   // flip Y (OpenGL vs ImGui)
        ImGui::TextDisabled("Jasny = blisko, ciemny = daleko (zlinearyzowany)");

        ImGui::Separator();

        // ── Lista swiatel ─────────────────────────────────────────────────────────
        if (ImGui::CollapsingHeader("GPULights"))
        {
            for (int i = 0; i < usedLayers; i++)
            {
                const GPULight& g = gpuLights[i];
                ImGui::PushID(i);
                char label[32]; snprintf(label, sizeof(label), "Light %d", i);
                if (ImGui::TreeNode(label))
                {
                    ImGui::Text("pos   (%.2f, %.2f, %.2f)  type=%.0f",
                        g.position.x, g.position.y, g.position.z, g.position.w);
                    ImGui::Text("dir   (%.2f, %.2f, %.2f)",
                        g.direction.x, g.direction.y, g.direction.z);
                    ImGui::Text("on=%.0f  intensity=%.2f  range=%.2f",
                        g.params2.z, g.params1.w, g.params2.w);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        ImGui::End();
    }



};

#endif