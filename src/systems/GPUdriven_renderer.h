#ifndef GPUDRIVEN_RENDERER_H
#define GPUDRIVEN_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

#include "material.h"
#include "shader.h"
#include "compute_shader.h"
#include "mesh_data.h"

struct DrawElementsIndirectCommand {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    uint32_t baseVertex;
    uint32_t baseInstance;
};

struct RenderData
{
    glm::mat4 modelMatrix;
    glm::vec4 aabbMin;
    glm::vec4 aabbMax;
    uint32_t meshID;
    uint32_t materialID;
    uint32_t skeletonID;
    uint32_t padding;
};

struct GPUMeshData
{
    uint32_t indexCount;
    uint32_t firstIndex;
    uint32_t baseVertex;
    uint32_t padding;
};

struct GPUMaterial
{
    GLuint64 diffuseHandle;
    GLuint64 specularHandle;
    GLuint64 normalHandle;
    uint32_t packedColor;
    float shininess;
    //uint32_t padding;
    //uint32_t padding2;
    //glm::vec4 diffuseColorAndShininess;
};


struct GPULight
{
    glm::vec4 position;   // xyz + type
    glm::vec4 direction;  // xyz + intensity

    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    glm::vec4 params1;
    // x = constant
    // y = linear
    // z = quadratic
    // w = range

    glm::vec4 params2;
    // x = cutOff
    // y = outerCutOff
    // z = enabled
    // w = unused
};


struct GPUMeshMeta {
    uint32_t instanceOffset;
    uint32_t instanceCount;
    uint32_t padding0;
    uint32_t padding1;
};

struct GPUInstanceData {
    glm::mat4 model;
    uint32_t  materialID;
    uint32_t  objectID;
    uint32_t skeletonID;
    uint32_t padding;
};
//
//// UBO per-frame — jeden transfer na klatkę
//struct FrameUBO {
//    glm::mat4 viewProjection;
//    glm::mat4 view;
//    glm::vec4 viewPos;      // xyz = pozycja, w = unused
//    glm::float32_t zNear;
//    glm::float32_t zFar;
//    glm::int32_t   numLights;
//    glm::int32_t   _pad;
//    glm::vec2      screenSize;
//    glm::vec2      _pad2;
//};

// UBO świateł
static constexpr int MAX_UBO_LIGHTS = 512;
struct LightsUBO {
    GPULight lights[MAX_UBO_LIGHTS];
};

static constexpr uint32_t MAX_BONES_PER_SKELETON = 200u;
static constexpr uint32_t NO_SKELETON = 0xFFFFFFFFu;


class GPUDrivenRenderer {
private:
    //rejestr geometria
    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;

    //rejestr
    std::vector<GPUMeshData> meshesData;
    std::vector<GPUMaterial> materials;
    //std::vector<GPULight> gpuLights;
    std::vector<GPUMeshMeta> meshMetaCPU;

    std::unordered_map<MeshData*, uint32_t> meshRegistry;
    std::unordered_map<Material*, uint32_t> materialRegistry;
    std::unordered_map<GLuint, GLuint64> handleCacheTextures;

    uint32_t instanceBufferCapacity = 64;
    int maxRenderObjects = 196;

    GLuint64 GetOrCreateHandle(GLuint texID) {
        if (texID == 0) return 0;
        auto it = handleCacheTextures.find(texID);
        if (it != handleCacheTextures.end()) return  it->second;

        GLuint64 handle = glGetTextureHandleARB(texID);
        glMakeTextureHandleResidentARB(handle);
        handleCacheTextures[texID] = handle;
        return handle;
    }

    void ResizeIfNeeded(int required) {
        if (required <= maxRenderObjects) return;
        maxRenderObjects = std::max(required * 2, 64);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderDataSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(RenderData), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCmdSSBO);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, maxRenderObjects * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

        spdlog::info("GPUDrivenRenderer: resize → {} obiektów", maxRenderObjects);
    }

    // Zmień rozmiar instanceSSBO jeśli potrzeba więcej miejsca
    void ResizeInstanceBufferIfNeeded(uint32_t required) {
        if (required <= instanceBufferCapacity) return;
        instanceBufferCapacity = required * 2; // 2x zapas

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instanceBufferCapacity * sizeof(GPUInstanceData), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }


public:
	GLuint VBO = 0;
	GLuint VAO = 0;
	GLuint EBO = 0;

    GLuint renderDataSSBO = 0; // bind 0
    GLuint meshCountersSSBO = 0; // bind 0
    GLuint meshMetaSSBO = 0; // bind 1
    GLuint totalVisibleSSBO = 0; // bind 2
    //build command
    GLuint instanceSSBO = 0; // bind 3
    GLuint drawCmdSSBO = 0; // bind 4
    //GLuint drawCountSSBO = 0; // bind 5
    GLuint meshDataSSBO = 0; // bind 6
    GLuint materialSSBO = 0; /// bind 7
    //GLuint lightsSSBO = 0;
    GLuint boneMatricesSSBO = 0;

    //GLuint frameUBO = 0;  // bind = 0
    //GLuint lightsUBO = 0;  // bind = 1
    
    uint32_t* totalVisibleMapped = nullptr;

    GLuint hizTexture = 0;
    int hizMipLevels = 0;
    int screenWidth = 0, screenHeight = 0;


    int vpWidth = 0, vpHeight = 0;

    ComputeShader* shaderHizCullCount = nullptr;
    ComputeShader* shaderPrefixSum = nullptr;
    ComputeShader* shaderHizWritePass = nullptr;
    ComputeShader* shaderBuildCmds = nullptr;
    ComputeShader* shaderHizDownsample = nullptr;
    Shader* shaderRender = nullptr;

    bool dirtyInstance = true;

	GPUDrivenRenderer() = default; 

    ~GPUDrivenRenderer() {
        if (totalVisibleMapped) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, totalVisibleSSBO);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            totalVisibleMapped = nullptr;
        }

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &renderDataSSBO);
        glDeleteBuffers(1, &meshCountersSSBO);
        glDeleteBuffers(1, &meshMetaSSBO);
        glDeleteBuffers(1, &totalVisibleSSBO);
        glDeleteBuffers(1, &instanceSSBO);
        glDeleteBuffers(1, &drawCmdSSBO);
        glDeleteBuffers(1, &meshDataSSBO);
        glDeleteBuffers(1, &materialSSBO);
        //glDeleteBuffers(1, &frameUBO);
        //glDeleteBuffers(1, &lightsUBO);
        glDeleteBuffers(1, &boneMatricesSSBO);
        if (hizTexture) glDeleteTextures(1, &hizTexture);
    }

    void ResizeBoneBufferIfNeeded(uint32_t skeletonCount)
    {
        if (skeletonCount == 0) return;

        size_t required = (size_t)skeletonCount * MAX_BONES_PER_SKELETON * sizeof(glm::mat4);
        size_t current = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneMatricesSSBO);
        glGetBufferParameteri64v(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, (GLint64*)&current);

        if (required > current)
        {
            glBufferData(GL_SHADER_STORAGE_BUFFER, required, nullptr, GL_DYNAMIC_DRAW);
            spdlog::info("BoneMatricesSSBO resize: {} szkieletów", skeletonCount);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

	void Init(int width, int height)
	{
        screenWidth = width;
        screenHeight = height;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

        glBindVertexArray(0);

        glGenBuffers(1, &renderDataSSBO);
        glGenBuffers(1, &meshCountersSSBO);
        glGenBuffers(1, &meshMetaSSBO);
        glGenBuffers(1, &instanceSSBO);
        // Indirect draw commands
        glGenBuffers(1, &drawCmdSSBO);
        // licznik draw calls
     /*   glGenBuffers(1, &drawCountSSBO);*/
        glGenBuffers(1, &meshDataSSBO);
        glGenBuffers(1, &materialSSBO);
     /*   glGenBuffers(1, &lightsSSBO);*/
        glGenBuffers(1, &boneMatricesSSBO);

        glGenBuffers(1, &totalVisibleSSBO);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instanceBufferCapacity * sizeof(GPUInstanceData), nullptr, GL_DYNAMIC_DRAW);
       
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
        //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneMatricesSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_BONES_PER_SKELETON * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, totalVisibleSSBO);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        totalVisibleMapped = (uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        spdlog::warn("inicjalizacja renderData");
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderDataSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(RenderData), nullptr, GL_DYNAMIC_DRAW);

        GLint size = 0;
        glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &size);

        std::cout << "AFTER INIT SIZE = " << size << std::endl;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        // UBO — światła (binding = 0)
        //glGenBuffers(1, &lightsUBO);
        //glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
        //glBufferData(GL_UNIFORM_BUFFER, sizeof(LightsUBO), nullptr, GL_DYNAMIC_DRAW);
        //glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightsUBO);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        InitHiZ(width, height);
    }

    //void InitHiZ(int width, int height)
    //{
    //    if (hizTexture) glDeleteTextures(1, &hizTexture);
    //    hizMipLevels = 1 + (int)std::floor(std::log2(std::max(width, height)));

    //    glGenTextures(1, &hizTexture);
    //    glBindTexture(GL_TEXTURE_2D, hizTexture);
    //    glTexStorage2D(GL_TEXTURE_2D, hizMipLevels, GL_DEPTH_COMPONENT32F, width, height);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //    glBindTexture(GL_TEXTURE_2D, 0);
    //}

    void InitHiZ(int w, int h)
    {
        hizMipLevels = static_cast<int>(std::floor(std::log2(std::max(w/2, h)))) + 1;

        //glGenTextures(1, &hizTexture);
        //glBindTexture(GL_TEXTURE_2D, hizTexture);
        //glTexStorage2D(GL_TEXTURE_2D, hizMipLevels, GL_DEPTH_COMPONENT32F, w, h); // ← zmiana
        //glTextureParameteri(hizTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        //glTextureParameteri(hizTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTextureParameteri(hizTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTextureParameteri(hizTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTextureParameteri(hizTexture, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        //glBindTexture(GL_TEXTURE_2D, 0);

        spdlog::info("RendererManager: HiZ {}x{} mips={}", w, h, hizMipLevels);
    }

    void clearRegisterAll()
    {
        allVertices.clear();
        allIndices.clear();
        meshesData.clear();
        materials.clear();
        meshMetaCPU.clear();
        meshRegistry.clear();
        materialRegistry.clear();
    }

    void AttachHiZ(GLuint tex, int mipLevels, int vpW, int vpH) {
        hizTexture = tex;
        hizMipLevels = mipLevels;
        vpWidth = vpW;
        vpHeight = vpH;
    }


    uint32_t RegisterMesh(MeshData* data)
    {
        auto it = meshRegistry.find(data);
        if (it != meshRegistry.end())
        {
            return it->second;
        }

        GPUMeshData meshData;
        meshData.indexCount = (uint32_t)data->indices.size();
        meshData.firstIndex = (uint32_t)allIndices.size();
        meshData.baseVertex = (uint32_t)allVertices.size();
        meshData.padding = 0;

        // vertices
        allVertices.insert(allVertices.end(), data->vertices.begin(), data->vertices.end());

        // indices
        allIndices.insert(allIndices.end(), data->indices.begin(), data->indices.end());

        meshesData.push_back(meshData);
        uint32_t id = meshesData.size() - 1;
        meshRegistry[data] = id;
        spdlog::error("Zarejestrowano mesh");
        spdlog::info(allVertices.size());

        return (GLuint)id; //meshID
    }

    uint32_t GetMeshId(MeshData* data) {
        auto it = meshRegistry.find(data);

        if (it != meshRegistry.end()) {
            return it->second;
        }

        return UINT32_MAX;
    }


    uint32_t RegisterMaterial(Material* mat) {
        auto it = materialRegistry.find(mat);
        if (it != materialRegistry.end()) return it->second;

        GPUMaterial gpu;
        gpu.diffuseHandle = GetOrCreateHandle(mat->diffuseMap);
        gpu.specularHandle = GetOrCreateHandle(mat->specularMap);
        gpu.normalHandle = GetOrCreateHandle(mat->normalMap);
        gpu.packedColor = packUnorm4x8(glm::vec4(mat->diffuseColor, 1.0f));
        gpu.shininess = mat->shininess;

        uint32_t id = (uint32_t)materials.size();
        materials.push_back(gpu);
        materialRegistry[mat] = id;

        spdlog::error("Zarejestrowano material");
        spdlog::info(materialRegistry.size());
        return id; //materialID
    }


    uint32_t GetMaterialId(Material* mat) {
        auto it = materialRegistry.find(mat);

        if (it != materialRegistry.end()) {
            return it->second;
        }

        return UINT32_MAX;
    }


    void UploadObjects(const std::vector<RenderData>& objects)
    {
        ResizeIfNeeded((int)objects.size());

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderDataSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, objects.size() * sizeof(RenderData), objects.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void UploadMeshes() {
        uint32_t meshCount = (uint32_t)meshesData.size();

        glBindVertexArray(VAO);
        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(Vertex), allVertices.data(), GL_STATIC_DRAW);

        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndices.size() * sizeof(unsigned int), allIndices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);

        // MeshData SSBO
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshDataSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(GPUMeshData), meshesData.data(), GL_STATIC_DRAW);

        //MeshCounters
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshCountersSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        //glNamedBufferData(meshCountersSSBO, meshCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        // MeshMeta
        meshMetaCPU.resize(meshCount, { 0, 0, 0, 0 });
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshMetaSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(GPUMeshMeta), meshMetaCPU.data(), GL_DYNAMIC_DRAW);

        // DrawCommands SSBO
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCmdSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        spdlog::critical("Dane sie wysylaja");
    }

    void UploadMaterials()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(GPUMaterial), materials.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        spdlog::critical("Materialy sie wysylaja");
    }

    //size_t prevSizeLights = 0;

    //void UpdateAndUploadLights(std::vector<LightComponent*>& lights, std::vector<TransformComponent*>& transforms)
    //{
    //    if (lights.empty()) return;

    //    uint32_t count = std::min((uint32_t)lights.size(), (uint32_t)MAX_UBO_LIGHTS);
    //    gpuLights.resize(count);

    //    for (size_t i = 0; i < count; i++)
    //    {
    //        LightComponent* light = lights[i];
    //        TransformComponent* transform = transforms[i];
    //        if (!light || !transform) continue;

    //        GPULight& g = gpuLights[i];

    //        const bool on = light->isOn;

    //        g.position = glm::vec4(transform->position, (float)light->type);
    //        g.direction = (glm::length2(light->direction) < 0.0001f) ? glm::vec4(TransformHelper::getForward(*transform), 0.0f) : glm::vec4(light->direction, 0.0f);

    //        const glm::vec3& zero = glm::vec3(0.0f);
    //        g.ambient = glm::vec4(on ? light->ambient : zero, 0.0f);
    //        g.diffuse = glm::vec4(on ? light->diffuse : zero, 0.0f);
    //        g.specular = glm::vec4(on ? light->specular : zero, 0.0f);
    //        g.params1 = glm::vec4(light->constant, light->linear, light->quadratic, 0.0f);
    //        g.params2 = glm::vec4(light->cutOff, light->outerCutOff, on ? 1.0f : 0.0f, 0.0f);
    //    }


    //    glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
    //    glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(GPULight), gpuLights.data());
    //    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //    //if (prevSizeLights == gpuLights.size())
    //    //{
    //    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsSSBO);
    //    //    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuLights.size() * sizeof(GPULight), gpuLights.data());
    //    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    //    //}
    //    //else
    //    //{
    //    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsSSBO);
    //    //    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuLights.size() * sizeof(GPULight), gpuLights.data(), GL_DYNAMIC_DRAW);
    //    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    //    //}

    //    prevSizeLights = count;
    //}

    void UploadAllBoneMatrices(const std::vector<glm::mat4>& allBones)
    {
        if (allBones.empty()) return;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneMatricesSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, allBones.size() * sizeof(glm::mat4), allBones.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ResetMeshCounters()
    {
        uint32_t meshCount = (uint32_t)meshesData.size();
        // Jeden clear zamiast N subData — szybsze
        uint32_t zero = 0;
        glClearNamedBufferData(meshCountersSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); // nullptr = wypełnij zerami
    }

    void DispatchCountPass(uint32_t objectCount)
    {
        shaderHizCullCount->use();
        glUniform1ui(glGetUniformLocation(shaderHizCullCount->ID, "objectCount"), objectCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, renderDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, meshCountersSSBO);

        glDispatchCompute((objectCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    
    void DispatchPrefixSum(uint32_t objectCount)
    {
        shaderPrefixSum->use();

        uint32_t meshCount = (uint32_t)meshesData.size();
        glUniform1ui(glGetUniformLocation(shaderPrefixSum->ID, "meshCount"), meshCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshCountersSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, totalVisibleSSBO); // totalVisible

        glDispatchCompute((meshCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
        ResizeInstanceBufferIfNeeded(objectCount);
    }



    void DispatchWritePass(const glm::mat4& viewProj, uint32_t objectCount)
    {
        shaderHizWritePass->use();
        spdlog::warn("hizMipLevels");
        spdlog::warn(hizMipLevels);
        glUniformMatrix4fv(glGetUniformLocation(shaderHizWritePass->ID, "viewProjection"), 1, GL_FALSE, &viewProj[0][0]);
        glUniform2f(glGetUniformLocation(shaderHizWritePass->ID, "screenSize"), (float)(vpWidth > 0 ? vpWidth : screenWidth), (float)(vpHeight > 0 ? vpHeight : screenHeight));
        glUniform1i(glGetUniformLocation(shaderHizWritePass->ID, "hizMipLevels"), hizTexture ? hizMipLevels : 0); // 0 = wyłącz HiZ
        glUniform1ui(glGetUniformLocation(shaderHizWritePass->ID, "objectCount"), objectCount);
        glUniform1i(glGetUniformLocation(shaderHizWritePass->ID, "enableFrustumCulling"), GL_TRUE);

        glTextureParameteri(hizTexture, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glBindTextureUnit(0, hizTexture);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instanceSSBO);

        glDispatchCompute((objectCount + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void DispatchBuildCommands()
    {
        uint32_t meshCount = (uint32_t)meshesData.size();

        shaderBuildCmds->use();
        glUniform1ui(glGetUniformLocation(shaderBuildCmds->ID, "meshCount"), meshCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawCmdSSBO);

        glDispatchCompute((meshCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }

    //void CopyDepthToHiZ(GLuint depthTexture)
    //{
    //    glCopyImageSubData(depthTexture, GL_TEXTURE_2D, 0, 0, 0, 0, hizTexture, GL_TEXTURE_2D, 0, 0, 0, 0, screenWidth, screenHeight, 1);
    //}

    void CopyDepthToHiZ(GLuint depthTexture)
    {
        glCopyImageSubData(depthTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
            hizTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
            screenWidth, screenHeight, 1);
    }

    //void BuildHiZ(GLuint depthTexture)
    //{
    //    int w = vpWidth > 0 ? vpWidth : screenWidth;
    //    int h = vpHeight > 0 ? vpHeight : screenHeight;

    //    // Krok 0: skopiuj depth 1:1 do mip0 hizTexture
    //    glCopyImageSubData(depthTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
    //        hizTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
    //        w, h, 1);
    //    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

    //    // Krok 1+: downsampling mip1, mip2, ...
    //    shaderHizDownsample->use();
    //    glTextureParameteri(hizTexture, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    //    int mw = w, mh = h;
    //    for (int mip = 1; mip < hizMipLevels; mip++) {
    //        mw = std::max(1, mw / 2);
    //        mh = std::max(1, mh / 2);

    //        glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, mip - 1);
    //        glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, mip - 1);
    //        glBindTextureUnit(0, hizTexture);                                             // prevMip
    //        glBindImageTexture(1, hizTexture, mip, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F); // currentMip

    //        glDispatchCompute((mw + 7) / 8, (mh + 7) / 8, 1);
    //        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    //    }

    //    glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, 0);
    //    glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, hizMipLevels - 1);
    //}


    void BuildHiZ(GLuint depthTexture)
    {
        int w = vpWidth > 0 ? vpWidth : screenWidth;
        int h = vpHeight > 0 ? vpHeight : screenHeight;

        shaderHizDownsample->use();

        // Krok 0: depth texture → hiz mip0 przez compute (nie glCopyImageSubData!)
        // depth ma GL_DEPTH_COMPONENT32F, hiz ma GL_R32F — różne klasy, copy jest GL_INVALID_OPERATION
        glTextureParameteri(depthTexture, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glBindTextureUnit(0, depthTexture);
        glBindImageTexture(1, hizTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        glDispatchCompute((w + 7) / 8, (h + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        // Krok 1+: downsampling mip1, mip2, ...
        glTextureParameteri(hizTexture, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        int mw = w, mh = h;
        for (int mip = 1; mip < hizMipLevels; mip++) {
            mw = std::max(1, mw / 2);
            mh = std::max(1, mh / 2);

            glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, mip - 1);
            glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, mip - 1);
            glBindTextureUnit(0, hizTexture);
            glBindImageTexture(1, hizTexture, mip, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

            glDispatchCompute((mw + 7) / 8, (mh + 7) / 8, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, hizMipLevels - 1);
    }

    //void BuildHiZ()
    //{
    //    shaderHizDownsample->use();

    //    int w = screenWidth, h = screenHeight;

    //    for (int mip = 1; mip < hizMipLevels; mip++) {
    //        w = std::max(1, w / 2);
    //        h = std::max(1, h / 2);

    //        // Poprzedni mip jako sampler (texture view na mip-1)
    //        // Ograniczamy BASE/MAX żeby sampler czytał tylko ten poziom
    //        glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, mip - 1);
    //        glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, mip - 1);
    //        glBindTextureUnit(0, hizTexture);

    //        // Aktualny mip jako image2D (zapis)
    //        glBindImageTexture(1, hizTexture, mip, GL_FALSE, 0,
    //            GL_WRITE_ONLY, GL_R32F);

    //        glDispatchCompute((w + 7) / 8, (h + 7) / 8, 1);
    //        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
    //            | GL_TEXTURE_FETCH_BARRIER_BIT);
    //    }

    //    // Przywróć pełny zakres mipów
    //    glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, 0);
    //    glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, hizMipLevels - 1);
    //}



    void BindForDraw()
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, instanceSSBO); // vertex shader
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, boneMatricesSSBO); // vertex shader
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, materialSSBO);   // fragment shader
    }


    void Draw()
    {
        BindForDraw();
        glBindVertexArray(VAO);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCmdSSBO);

        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshesData.size(), sizeof(DrawElementsIndirectCommand));

        glBindVertexArray(0);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    }

    void BuildInstance(uint32_t objectCount)
    {
        // 2. Zeruj liczniki meshów
        ResetMeshCounters();

        // 3. COUNT PASS: culling + zliczanie (bez zapisu instancji)
        DispatchCountPass(objectCount);
        // barrier wewnątrz DispatchCountPass
        // 
        // 4. PREFIX-SUM na CPU (czyta tylko meshCount uint32!)
        // wewnątrz: glGetNamedBufferSubData + wysyłka MeshMeta + ewentualny resize
        DispatchPrefixSum(objectCount);
        dirtyInstance = false;
    }


    void RenderFrame(const glm::mat4& viewProj, const std::vector<RenderData>& objects, GLuint depthTexturePrevFrame, glm::vec3 currentCameraPos)
    {
        uint32_t objCount = (uint32_t)objects.size();

        // 0. Aktualizuj obiekty na GPU
        UploadObjects(objects);
        //UploadLights();
        //DebugReadBuffers(objCount, (uint32_t)meshesData.size());

        // 1. Zbuduj HiZ z depth poprzedniej klatki
        if (depthTexturePrevFrame != 0) {
            //CopyDepthToHiZ(depthTexturePrevFrame);
            BuildHiZ(depthTexturePrevFrame);
        }
        if (dirtyInstance)
            BuildInstance(objCount);
        //DebugPipelineState(objCount);
  
        // 5. WRITE PASS: zapis instancji
        DispatchWritePass(viewProj, objCount);
        // barrier wewnątrz DispatchWritePass

        // 6. BUILD COMMANDS: DrawIndirect per mesh
        DispatchBuildCommands();
        // barrier wewnątrz DispatchBuildCommands

        shaderRender->use();
        //shaderRender->setMat4("viewProjection", viewProj);
        //shaderRender->setVec3("viewPos", currentCameraPos);
        //shaderRender->setInt("numLights", (int)gpuLights.size());
        //DebugPipelineState(objCount);
        // 7. Rysuj
        Draw();
    }


    void DebugShowHiZ(int mipLevel = 0)
    {
        if (!hizTexture) return;

        // Slider PRZED odczytem — żeby mipLevel byl aktualny
        static int selectedMip = 0;
        ImGui::Begin("HiZ Debug");
        if (ImGui::SliderInt("Mip Level", &selectedMip, 0, hizMipLevels - 1))
        {
            // zmiana mipa — debugW/H reset żeby wymusic realloc tekstury
            static int& dW = *([]() -> int* { static int x = 0; return &x; }());
            static int& dH = *([]() -> int* { static int x = 0; return &x; }());
            dW = dH = 0;
        }
        mipLevel = selectedMip;

        int mipW = std::max(1, (vpWidth > 0 ? vpWidth : screenWidth) >> mipLevel);
        int mipH = std::max(1, (vpHeight > 0 ? vpHeight : screenHeight) >> mipLevel);

        // Pobierz dane — format musi zgadzac sie z formatem tekstury
        // hizTexture pochodzi z GPUDrivenManager::InitHiZ = GL_R32F
        // wiec czytamy GL_RED, nie GL_DEPTH_COMPONENT
        std::vector<float> pixels(mipW * mipH);
        glGetTextureImage(hizTexture, mipLevel,
            GL_RED, GL_FLOAT,   // ← GL_RED dla R32F
            (GLsizei)(pixels.size() * sizeof(float)),
            pixels.data());

        float dMin = *std::min_element(pixels.begin(), pixels.end());
        float dMax = *std::max_element(pixels.begin(), pixels.end());
        float range = (dMax - dMin) > 0.0001f ? (dMax - dMin) : 1.0f;

        std::vector<uint8_t> rgb(mipW * mipH * 3);
        for (int i = 0; i < mipW * mipH; i++) {
            float n = 1.0f - ((pixels[i] - dMin) / range);
            uint8_t v = (uint8_t)(glm::clamp(n, 0.0f, 1.0f) * 255.0f);
            rgb[i * 3 + 0] = v;
            rgb[i * 3 + 1] = v;
            rgb[i * 3 + 2] = v;
        }

        static GLuint debugTex = 0;
        static int debugW = 0, debugH = 0;
        if (!debugTex) {
            glGenTextures(1, &debugTex);
            glBindTexture(GL_TEXTURE_2D, debugTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBindTexture(GL_TEXTURE_2D, debugTex);
        if (debugW != mipW || debugH != mipH) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mipW, mipH, 0,
                GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            debugW = mipW;
            debugH = mipH;
        }
        else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mipW, mipH,
                GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        ImGui::Text("Mip %d: %dx%d  depth [%.4f, %.4f]",
            mipLevel, mipW, mipH, dMin, dMax);

        float dispW = std::min((float)mipW, 512.0f);
        float dispH = dispW * ((float)mipH / (float)mipW);
        ImGui::Image(
            (ImTextureID)(intptr_t)debugTex,
            ImVec2(dispW, dispH),
            ImVec2(0, 1),   // uv0 — lewy górny = dół tekstury
            ImVec2(1, 0)    // uv1 — prawy dolny = góra tekstury
        );

        ImGui::End(); // dokładnie jedno End na Begin powyżej
    }


    void DebugPipelineState(uint32_t objCount)
    {
        // ── 1. Czy w ogóle są meshe i obiekty? ───────────────────────
        spdlog::warn("=== GPU PIPELINE DEBUG ===");
        spdlog::warn("meshCount={} objCount={}", meshesData.size(), objCount);

        if (meshesData.empty()) {
            spdlog::error("BRAK MESHÓW — UploadMeshes() nie zostało wywołane lub RegisterMesh() nie dodało nic");
            return;
        }
        if (objCount == 0) {
            spdlog::error("BRAK OBIEKTÓW — CollectRenderData() zwróciło pusty wektor");
            return;
        }

        uint32_t meshCount = (uint32_t)meshesData.size();

        // ── 2. Czy meshCountersSSBO ma niezerowe wartości po CountPass? ──
        {
            std::vector<uint32_t> counters(meshCount);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshCountersSSBO);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, meshCount * sizeof(uint32_t), counters.data());
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            spdlog::warn("--- meshCounters po CountPass ---");
            for (uint32_t i = 0; i < meshCount; i++)
                spdlog::warn("  mesh[{}] count={}", i, counters[i]);
        }

        // ── 3. Czy meshMeta ma poprawne offsety po PrefixSum? ────────
        {
            std::vector<GPUMeshMeta> meta(meshCount);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshMetaSSBO);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, meshCount * sizeof(GPUMeshMeta), meta.data());
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            spdlog::warn("--- meshMeta po PrefixSum ---");
            for (uint32_t i = 0; i < meshCount; i++)
                spdlog::warn("  mesh[{}] instanceOffset={} instanceCount={}", i, meta[i].instanceOffset, meta[i].instanceCount);
        }

        // ── 4. Czy totalVisible > 0? ─────────────────────────────────
        if (totalVisibleMapped)
            spdlog::warn("totalVisible (mapped)={}", *totalVisibleMapped);

        // ── 5. Czy instanceSSBO zawiera dane po WritePass? ───────────
        {
            uint32_t readCount = std::min(objCount, 4u);
            std::vector<GPUInstanceData> instances(readCount);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, readCount * sizeof(GPUInstanceData), instances.data());
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            spdlog::warn("--- pierwsze {} instancji po WritePass ---", readCount);
            for (uint32_t i = 0; i < readCount; i++)
                spdlog::warn("  inst[{}] matID={} skelID={} model[3]={},{},{}",
                    i, instances[i].materialID, instances[i].skeletonID,
                    instances[i].model[3][0], instances[i].model[3][1], instances[i].model[3][2]);
        }

        // ── 6. Czy DrawCommands są poprawne? ─────────────────────────
        {
            std::vector<DrawElementsIndirectCommand> cmds(meshCount);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCmdSSBO);
            glGetBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, meshCount * sizeof(DrawElementsIndirectCommand), cmds.data());
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
            spdlog::warn("--- DrawCommands po BuildCommands ---");
            for (uint32_t i = 0; i < meshCount; i++)
                spdlog::warn("  cmd[{}] count={} instanceCount={} firstIndex={} baseVertex={} baseInstance={}",
                    i, cmds[i].count, cmds[i].instanceCount,
                    cmds[i].firstIndex, cmds[i].baseVertex, cmds[i].baseInstance);
        }

        // ── 7. Czy VAO/VBO mają dane? ────────────────────────────────
        {
            GLint vboSize = 0;
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            spdlog::warn("VBO size={} bytes (oczekiwano={})", vboSize, (int)(allVertices.size() * sizeof(Vertex)));

            GLint eboSize = 0;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            spdlog::warn("EBO size={} bytes (oczekiwano={})", eboSize, (int)(allIndices.size() * sizeof(uint32_t)));
        }

        // ── 8. Czy materialSSBO ma dane? ─────────────────────────────
        {
            GLint matSize = 0;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
            glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &matSize);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            spdlog::warn("materialSSBO size={} bytes (oczekiwano={})", matSize, (int)(materials.size() * sizeof(GPUMaterial)));
        }

        // ── 9. Błędy OpenGL ──────────────────────────────────────────
        {
            GLenum err;
            bool anyErr = false;
            while ((err = glGetError()) != GL_NO_ERROR) {
                spdlog::error("GL ERROR: 0x{:X}", err);
                anyErr = true;
            }
            if (!anyErr) spdlog::warn("Brak błędów OpenGL");
        }

        spdlog::warn("=== KONIEC DEBUG ===");
    }


    uint32_t GetMeshCount()    const { return (uint32_t)meshesData.size(); }
    uint32_t GetMaterialCount()const { return (uint32_t)materials.size(); }
    int      GetHiZMipLevels() const { return hizMipLevels; }
};

#endif



//void ResetDrawCount() {
//    uint32_t zero = 0;
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
//    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
//}

//void BindForCompute() {
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderDataSSBO);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshDataSSBO);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawCmdSSBO);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, drawCountSSBO);
//}


    //uint32_t ReadDrawCount() {
    //    uint32_t count = 0;
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
    //    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &count);
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    //    return count;
    //}

    //void ComputePrefixSum()
    //{
    //    uint32_t meshCount = (uint32_t)meshesData.size();

    //    // Odczytaj liczniki z GPU
    //    std::vector<uint32_t> counts(meshCount);
    //    glGetNamedBufferSubData(meshCountersSSBO, 0, meshCount * sizeof(uint32_t), counts.data());

    //    // Prefix-sum: instanceOffset[i] = sum(counts[0..i-1])
    //    uint32_t offset = 0; // offset = łączna liczba widocznych instancji w tej klatce
    //    for (uint32_t i = 0; i < meshCount; i++) {
    //        meshMetaCPU[i].instanceOffset = offset;
    //        meshMetaCPU[i].instanceCount = 0;   // reset przed write_pass
    //        offset += counts[i];
    //    }

    //    // Wyślij MeshMeta z powrotem na GPU
    //    glNamedBufferSubData(meshMetaSSBO, 0, meshCount * sizeof(GPUMeshMeta), meshMetaCPU.data());


    //    // Resize instanceSSBO jeśli potrzeba
    //    ResizeInstanceBufferIfNeeded(offset);
    //}
//struct DrawCommand {
//     uint32_t count;
//     uint32_t instanceCount;
//     uint32_t firstIndex;
//     uint32_t baseVertex;
//     uint32_t baseInstance;
// };
//
// GLuint vbo = 0;
// GLuint ebo = 0;
// GLuint meshInfoSSBO = 0;
// GLuint drawCmdSSBO = 0;
// GLuint compShader = 0;
// GLuint indirectBuffer = 0;
//
// struct MeshInfo {
//     uint32_t indexCount;
//     uint32_t firstIndex;
//     int32_t baseVertex;
//     uint32_t pad = 0;
// };
// 
// std::vector<MeshInfo> meshInfos;
//
// std::vector<Vertex> allVertices;
// std::vector<uint32_t> allIndices;
//
// GLuint RegisterMesh(const MeshData& data)
// {
//     uint32_t baseVertex = (uint32_t)allVertices.size();
//     uint32_t firstIndex = (uint32_t)allIndices.size();
//
//     // vertices
//     allVertices.insert(allVertices.end(),
//         data.vertices.begin(),
//         data.vertices.end());
//
//     // indices (BEZ OFFSETU)
//     allIndices.insert(allIndices.end(),
//         data.indices.begin(),
//         data.indices.end());
//
//     MeshInfo info;
//     info.indexCount = (uint32_t)data.indices.size();
//     info.firstIndex = firstIndex;
//     info.baseVertex = baseVertex;
//
//     meshInfos.push_back(info);
//
//     return (GLuint)meshInfos.size() - 1;
// }
//
// void InitGPU()
// {
//     compShader = ComputeShader("res/shaders/shader.comp").ID;
//
//     glGenBuffers(1, &vbo);
//     glGenBuffers(1, &ebo);
//     glGenBuffers(1, &meshInfoSSBO);
//     glGenBuffers(1, &indirectBuffer);
//
//     // VBO
//     glBindBuffer(GL_ARRAY_BUFFER, vbo);
//     glBufferData(GL_ARRAY_BUFFER,
//         allVertices.size() * sizeof(Vertex),
//         allVertices.data(),
//         GL_STATIC_DRAW);
//
//     // EBO
//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
//     glBufferData(GL_ELEMENT_ARRAY_BUFFER,
//         allIndices.size() * sizeof(uint32_t),
//         allIndices.data(),
//         GL_STATIC_DRAW);
//
//     // MeshInfo SSBO
//     glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshInfoSSBO);
//     glBufferData(GL_SHADER_STORAGE_BUFFER,
//         meshInfos.size() * sizeof(MeshInfo),
//         meshInfos.data(),
//         GL_STATIC_DRAW);
//
//     glUseProgram(compShader);
//
//     //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, objectsSSBO);
//     glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshInfoSSBO);
//
//     glDispatchCompute(1, 1, 1);
//
//     glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
// }
//
// void DrawGPU()
// {
//     glBindVertexArray(VAO);
//
//     glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
//
//     glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, 1, 0);
// }