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
    Transparent,
};

struct RenderPassConfig {
    RenderPassType  type;
    int sortOrder;      // kolejność wykonania
    bool depthWrite;
    bool blendingEnabled;
    Shader* shader = nullptr;  // może być nullptr
};

struct PassEntry {
    uint32_t passID;
    RenderPassConfig config;
    GPUDrivenRenderer renderer;
    std::vector<RenderData> objects;
};

struct FrameUBO {
    glm::mat4 viewProjection;
    glm::vec4 viewPos;   // xyz=kamera, w=unused
    float     zNear;
    float     zFar;
    int       numLights;
    int       _pad;
};

struct TransparentEntry {
    RenderData data;
    float      distanceSq;
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
    std::unordered_map<MeshData*, uint32_t> meshRegistry;
    std::unordered_map<Material*, uint32_t> materialRegistry;

    // --- passy (posortowane po sortOrder) ---
    std::vector<GPULight> gpuLights;
    std::vector<PassEntry> passes;

    std::unordered_map<Shader*, uint32_t> opaquePassByShader;
    std::unordered_map<Shader*, uint32_t> transparentPassByShader;

    ComputeShader* shaderCountInstance = nullptr;
    ComputeShader* shaderPrefixSum = nullptr;
    ComputeShader* shaderHizWritePass = nullptr;
    ComputeShader* shaderBuildCmds = nullptr;
    ComputeShader* shaderHizDownsample = nullptr;
    Shader* defaultShaderRender = nullptr;

    std::unordered_map<AnimatorComponent*, uint32_t> animatorIDMap;
    std::vector<glm::mat4> boneMatricesCache;


    void Init(int w, int h)
    {
        screenWidth = w;
        screenHeight = h;

        shaderCountInstance = new ComputeShader("res/shaders/hiz_culling_count.comp");
        shaderPrefixSum = new ComputeShader("res/shaders/prefix_sum.comp");
        shaderHizWritePass = new ComputeShader("res/shaders/write_pass.comp");
        shaderBuildCmds = new ComputeShader("res/shaders/build_commands.comp");
        shaderHizDownsample = new ComputeShader("res/shaders/hiz_build.comp");
        defaultShaderRender = new Shader("res/shaders/gpu_driven.vert", "res/shaders/gpu_driven.frag");

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

                r->RegisterMesh(mesh.cpuData.get());
                r->RegisterMaterial(mat);
            }
        }

        // Wyślij geometrię i materiały każdego pasa na GPU
        for (auto& entry : passes) {
            entry.renderer.UploadMeshes();
            entry.renderer.UploadMaterials();
        }

        spdlog::info("RendererManager: zainicjalizowano {} passów", passes.size());
    }



    void CollectRenderData(uint32_t passID, Query<TransformComponent, RenderComponent>& renderQuery, const glm::vec3& cameraPos)
    {
        PassEntry* entry = FindPass(passID);
        if (!entry) return;

        GPUDrivenRenderer* r = &entry->renderer;
        const SurfaceType   filter = PassTypeToSurface(entry->config.type);
        const bool          isTransparent = (entry->config.type == RenderPassType::Transparent);
        Shader* passShader = entry->config.shader ? entry->config.shader : defaultShaderRender;

        auto& transforms = std::get<0>(renderQuery.componentsVectors);
        auto& renderers = std::get<1>(renderQuery.componentsVectors);
        const size_t count = renderQuery.gameobjects.size();

        // ── 1. Nowe animatory ─────────────────────────────────────
        for (size_t i = 0; i < count; ++i) {
            const RenderComponent* rc = renderers[i];
            if (!rc || !rc->animator) continue;
            if (animatorIDMap.find(rc->animator) == animatorIDMap.end())
                animatorIDMap[rc->animator] = (uint32_t)animatorIDMap.size();
        }

        // ── 2. Rozmiar bufora kości ────────────────────────────────
        const size_t requiredBones = animatorIDMap.size() * MAX_BONES_PER_SKELETON;
        if (boneMatricesCache.size() != requiredBones)
            boneMatricesCache.resize(requiredBones, glm::mat4(1.0f));

        // ── 3. Macierze kości ─────────────────────────────────────
        for (size_t i = 0; i < count; ++i) {
            const RenderComponent* rc = renderers[i];
            if (!rc || !rc->animator || !rc->animator->currentSkeleton) continue;

            auto animIt = animatorIDMap.find(rc->animator);
            if (animIt == animatorIDMap.end()) continue;

            uint32_t slot = animIt->second;
            uint32_t boneCount = (uint32_t)std::min(
                rc->animator->finalBoneMatrices.size(),
                (size_t)MAX_BONES_PER_SKELETON);

            std::copy(
                rc->animator->finalBoneMatrices.begin(),
                rc->animator->finalBoneMatrices.begin() + boneCount,
                boneMatricesCache.begin() + slot * MAX_BONES_PER_SKELETON);
        }

        // ── 4. Buduj RenderData ───────────────────────────────────
        std::vector<TransparentEntry> transparentBuffer;

        entry->objects.clear();
        entry->objects.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            const TransformComponent* t = transforms[i];
            const RenderComponent* rc = renderers[i];
            if (!t || !rc) continue;

            const glm::mat4 model = t->modelMatrix;

            auto animIt = rc->animator
                ? animatorIDMap.find(rc->animator)
                : animatorIDMap.end();

            for (const auto& mesh : rc->meshes) {
                if (!mesh.cpuData || !mesh.material) continue;

                Material* mat = mesh.material.get();
                Shader* shader = mat->shader ? mat->shader : defaultShaderRender;

                // Filtruj: ten pass obsługuje tylko swój shader i swój surfaceType
                if (shader != passShader)     continue;
                if (mat->surfaceType != filter) continue;

                uint32_t meshID = r->GetMeshId(mesh.cpuData.get());
                if (meshID == UINT32_MAX) continue;

                uint32_t matID = r->GetMaterialId(mat);
                if (matID == UINT32_MAX) continue;

                const auto& aabb = mesh.cpuData->aabb;

                RenderData rd{
                    .modelMatrix = model,
                    .aabbMin = glm::vec4(aabb.min, 0.0f),
                    .aabbMax = glm::vec4(aabb.max, 0.0f),
                    .meshID = meshID,
                    .materialID = matID,
                    .skeletonID = (animIt != animatorIDMap.end())
                                   ? animIt->second : NO_SKELETON,
                    .padding = 0
                };

                if (isTransparent) {
                    glm::vec3 worldCenter = glm::vec3(model * glm::vec4(aabb.centerLocal, 1.0f));
                    float distSq = glm::length2(cameraPos - worldCenter);
                    transparentBuffer.push_back({ rd, distSq });
                }
                else {
                    entry->objects.push_back(rd);
                }
            }
        }

        // ── 5. Transparent: sort back-to-front ────────────────────
        if (isTransparent && !transparentBuffer.empty()) {
            std::sort(transparentBuffer.begin(), transparentBuffer.end(),
                [](const TransparentEntry& a, const TransparentEntry& b) {
                    return a.distanceSq > b.distanceSq;
                });
            entry->objects.reserve(transparentBuffer.size());
            for (auto& te : transparentBuffer)
                entry->objects.push_back(te.data);
        }

        // ── 6. Kości na GPU ───────────────────────────────────────
        r->ResizeBoneBufferIfNeeded((uint32_t)animatorIDMap.size());
        r->UploadAllBoneMatrices(boneMatricesCache);

        entry->renderer.dirtyInstance = true;
    }


    void CollectAllPasses(Query<TransformComponent, RenderComponent>& renderQuery, const glm::vec3& cameraPos)
    {
        for (auto& entry : passes)
            CollectRenderData(entry.passID, renderQuery, cameraPos);
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

        entry.renderer.Init(screenWidth, screenHeight);
        entry.renderer.AttachHiZ(hizTexture, hizMipLevels);
        entry.renderer.shaderHizCullCount = shaderCountInstance;
        entry.renderer.shaderPrefixSum = shaderPrefixSum;
        entry.renderer.shaderHizWritePass = shaderHizWritePass;
        entry.renderer.shaderBuildCmds = shaderBuildCmds;
        entry.renderer.shaderHizDownsample = shaderHizDownsample;
        entry.renderer.shaderRender = cfg.shader ? cfg.shader : defaultShaderRender;
        entry.renderer.lightsUBO = lightsUBO;
        spdlog::info("Add pass");
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
            if (e.passID == passID) return &e.renderer;
        return nullptr;
    }

    // Wygodne wrappery: rejestracja zasobu w konkretnym pasie
    uint32_t RegisterMeshInPass(uint32_t passID, MeshData* d)
    {
        GPUDrivenRenderer* r = GetRenderer(passID);
        if (!r) return UINT32_MAX;
        return r->RegisterMesh(d);
    }

    uint32_t RegisterMaterialInPass(uint32_t passID, Material* m)
    {
        GPUDrivenRenderer* r = GetRenderer(passID);
        if (!r) return UINT32_MAX;
        return r->RegisterMaterial(m);
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
                e.renderer.dirtyInstance = true;
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

        for (uint32_t i = 0; i < count; i++) {
            LightComponent* light = lights[i];
            TransformComponent* transform = transforms[i];
            if (!light || !transform) continue;

            GPULight& g = gpuLights[i];
            const bool on = light->isOn;
            const glm::vec3 zero(0.0f);

            g.position = glm::vec4(transform->position, (float)light->type);
            g.direction = (glm::length2(light->direction) < 0.0001f) ? glm::vec4(TransformHelper::getForward(*transform), 0.0f) : glm::vec4(light->direction, 0.0f);
            g.ambient = glm::vec4(on ? light->ambient : zero, 0.0f);
            g.diffuse = glm::vec4(on ? light->diffuse : zero, 0.0f);
            g.specular = glm::vec4(on ? light->specular : zero, 0.0f);
            g.params1 = glm::vec4(light->constant, light->linear, light->quadratic, 0.0f);
            g.params2 = glm::vec4(light->cutOff, light->outerCutOff, on ? 1.0f : 0.0f, 0.0f);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(GPULight), gpuLights.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    // Główna pętla renderowania
    void RenderFrame(const glm::mat4& viewProj, glm::vec3 cameraPos, GLuint prevDepth, float zNear = 0.1f, float zFar = 1000.0f)
    {
        const int numLights = (int)gpuLights.size();
        UploadFrameUBO(viewProj, cameraPos, numLights, zNear, zFar);

        //if (prevDepth && hizTexture) {
        //    glCopyImageSubData(prevDepth, GL_TEXTURE_2D, 0, 0, 0, 0,
        //        hizTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
        //        screenWidth, screenHeight, 1);
        //    BuildHiZ();
        //}
        spdlog::info("Renderowanie");
        for (auto& entry : passes) {
            if (entry.objects.empty()) continue;
            ApplyPassState(entry.config);
            cout << "renderuje sie " << endl;
            entry.renderer.RenderFrame(viewProj, entry.objects, prevDepth, cameraPos);
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

    void UploadFrameUBO(const glm::mat4& viewProj, const glm::vec3& cameraPos, int numLights, float zNear, float zFar)
    {
        FrameUBO data{};
        data.viewProjection = viewProj;
        data.viewPos = glm::vec4(cameraPos, 1.0f);
        data.zNear = zNear;
        data.zFar = zFar;
        data.numLights = numLights;

        glBindBuffer(GL_UNIFORM_BUFFER, frameUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FrameUBO), &data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


    void BuildHiZ()
    {
        for (auto& entry : passes) {
            if (entry.renderer.shaderHizDownsample) {
                entry.renderer.BuildHiZ();
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
};

#endif