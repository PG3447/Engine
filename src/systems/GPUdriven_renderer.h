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
#include <numeric>

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
    uint32_t  pad0;
    uint32_t  pad1;
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

// ============================================================
// Tabela buforów SSBO — co idzie gdzie
// ============================================================
//
//  Shader               binding  bufor
//  ──────────────────────────────────────────────────────────
//  hiz_culling_count    tex 0    hizTexture (sampler)
//                       1        renderDataSSBO
//                       2        meshCountersSSBO
//                       3        visibleFlagsSSBO
//                       4        sortKeysSSBO          (ping A)
//
//  histogram            0        sortKeysSSBO          (ping A / B)
//                       1        histogramSSBO
//
//  global_prefix        0        histogramSSBO
//                       1        globalPrefixSSBO
//
//  scatter              0        sortKeysSSBO  (in)    (ping)
//                       1        sortValuesSSBO(in)    (ping)
//                       2        histogramSSBO
//                       3        globalPrefixSSBO
//                       4        sortKeysSSBO  (out)   (pong)
//                       5        sortValuesSSBO(out)   (pong)
//   → ping/pong swap po każdym z 4 passów (bity 0,8,16,24)
//
//  prefix_sum           0        renderDataSSBO        (Objects)
//                       1        sortValuesSSBO        (SortedIndices — wynik sort)
//                       2        sortKeysSSBO          (SortedKeys    — wynik sort)
//                       3        groupBoundariesSSBO   (groupStart[])
//                       4        totalGroupsSSBO
//
//  write_pass           0        renderDataSSBO
//                       1        sortValuesSSBO        (SortedIndices)
//                       2        instanceSSBO
//
//  build_commands       0        meshDataSSBO
//                       1        sortKeysSSBO          (SortedKeys)
//                       2        groupBoundariesSSBO   (groupStart[])
//                       3        totalGroupsSSBO
//                       4        drawCmdSSBO
//
//  render (vertex)      3        instanceSSBO
//  render (fragment)    7        materialSSBO
// ============================================================


class GPUDrivenRenderer {
private:
    //rejestr geometria
    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;

    //rejestr
    std::vector<GPUMeshData> meshesData;
    std::vector<GPUMaterial> materials;
    std::vector<GPULight> gpuLights;
    std::vector<GPUMeshMeta> meshMetaCPU;

    std::unordered_map<Material*, uint32_t> materialRegistry;
    std::unordered_map<LightComponent*, uint32_t> lightRegistry;
    std::unordered_map<GLuint, GLuint64> handleCacheTextures;

    uint32_t instanceBufferCapacity = 64;
    int maxRenderObjects = 64;

    // Liczba workgroup użyta w ostatnim dispatchu histogram.comp
    // — potrzebna jako uniform groupCount dla global_prefix.comp
    uint32_t lastHistogramGroups = 0;


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

        // sortKeys/Values muszą pomieścić tyle samo elementów co renderData
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortKeysA_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortKeysB_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortValuesA_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortValuesB_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        uint32_t maxGroups = (maxRenderObjects + 255) / 256;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxGroups * 256 * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);


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

    // ---------- SSBO ----------
    GLuint renderDataSSBO = 0; // obiekty sceny
    GLuint meshCountersSSBO = 0; // per-mesh liczniki (reset każdą klatkę)
    GLuint visibleFlagsSSBO = 0; // 0/1 per obiekt — wynik cullingu
    GLuint meshDataSSBO = 0; // GPUMeshData (indexCount, firstIndex, baseVertex)
    GLuint materialSSBO = 0; // GPUMaterial
    GLuint lightsSSBO = 0; // GPULight
    GLuint instanceSSBO = 0; // GPUInstanceData — wejście vertex shadera
    GLuint drawCmdSSBO = 0; // DrawElementsIndirectCommand

    // Radix sort — dwa ping-pong pary (klucze + wartości = indeksy)
    GLuint sortKeysA_SSBO = 0;
    GLuint sortKeysB_SSBO = 0;
    GLuint sortValuesA_SSBO = 0;
    GLuint sortValuesB_SSBO = 0;

    // Histogram i prefix dla radix sort
    GLuint histogramSSBO = 0; // [groupCount * 256]
    GLuint globalPrefixSSBO = 0; // [256] — exclusive prefix sum

    // Granice grup (meshID, materialID) po sortowaniu
    GLuint groupBoundariesSSBO = 0; // groupStart[] — binding 3 w prefix_sum / build_commands
    GLuint totalGroupsSSBO = 0; // uint — łączna liczba unikalnych grup

    // ---------- UBO ----------
    GLuint frameUBO = 0;  // bind = 0
    GLuint lightsUBO = 0;  // bind = 1
    
    uint32_t* totalVisibleMapped = nullptr;

    GLuint hizTexture = 0;
    int hizMipLevels = 0;
    int screenWidth = 0, screenHeight = 0;

    ComputeShader* shaderHizCullCount = nullptr; // hiz_culling_count.comp
    ComputeShader* shaderHistogram = nullptr; // histogram.comp
    ComputeShader* shaderGlobalPrefix = nullptr; // global_prefix.comp
    ComputeShader* shaderScatter = nullptr; // scatter.comp
    ComputeShader* shaderPrefixSum = nullptr; // prefix_sum.comp  (granice grup)
    ComputeShader* shaderWritePass = nullptr; // write_pass.comp
    ComputeShader* shaderBuildCmds = nullptr; // build_commands.comp
    ComputeShader* shaderHizDownsample = nullptr; // hiz_build.comp
    Shader* shaderRender = nullptr;


	GPUDrivenRenderer() = default; 

    ~GPUDrivenRenderer() {
        //if (totalVisibleMapped) {
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, totalVisibleSSBO);
        //    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        //    totalVisibleMapped = nullptr;
        //}

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        //glDeleteBuffers(1, &renderDataSSBO);
        //glDeleteBuffers(1, &meshCountersSSBO);
        //glDeleteBuffers(1, &meshMetaSSBO);
        //glDeleteBuffers(1, &visibleFlagsSSBO);
        //glDeleteBuffers(1, &totalVisibleSSBO);
        //glDeleteBuffers(1, &instanceSSBO);
        //glDeleteBuffers(1, &drawCmdSSBO);
        ////glDeleteBuffers(1, &drawCountSSBO);
        //glDeleteBuffers(1, &meshDataSSBO);
        //glDeleteBuffers(1, &materialSSBO);
        //glDeleteBuffers(1, &frameUBO);
        //glDeleteBuffers(1, &lightsUBO);
        glDeleteBuffers(1, &renderDataSSBO);
        glDeleteBuffers(1, &meshCountersSSBO);
        glDeleteBuffers(1, &visibleFlagsSSBO);
        glDeleteBuffers(1, &meshDataSSBO);
        glDeleteBuffers(1, &materialSSBO);
        glDeleteBuffers(1, &lightsSSBO);
        glDeleteBuffers(1, &instanceSSBO);
        glDeleteBuffers(1, &drawCmdSSBO);
        glDeleteBuffers(1, &sortKeysA_SSBO);
        glDeleteBuffers(1, &sortKeysB_SSBO);
        glDeleteBuffers(1, &sortValuesA_SSBO);
        glDeleteBuffers(1, &sortValuesB_SSBO);
        glDeleteBuffers(1, &histogramSSBO);
        glDeleteBuffers(1, &globalPrefixSSBO);
        glDeleteBuffers(1, &groupBoundariesSSBO);
        glDeleteBuffers(1, &totalGroupsSSBO);
        glDeleteBuffers(1, &lightsUBO);
        if (hizTexture) glDeleteTextures(1, &hizTexture);
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
        
        //glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(Vertex), &data.vertices[0], GL_STATIC_DRAW);

        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), &data.indices[0], GL_STATIC_DRAW);
        spdlog::info("offsetof Normal: {}", offsetof(Vertex, Normal));
        spdlog::info("sizeof Vertex: {}", sizeof(Vertex));
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
        glGenBuffers(1, &visibleFlagsSSBO);
        glGenBuffers(1, &meshDataSSBO);
        glGenBuffers(1, &materialSSBO);
        glGenBuffers(1, &lightsSSBO);
        glGenBuffers(1, &instanceSSBO);
        glGenBuffers(1, &drawCmdSSBO);
        glGenBuffers(1, &sortKeysA_SSBO);
        glGenBuffers(1, &sortKeysB_SSBO);
        glGenBuffers(1, &sortValuesA_SSBO);
        glGenBuffers(1, &sortValuesB_SSBO);
        glGenBuffers(1, &histogramSSBO);
        glGenBuffers(1, &globalPrefixSSBO);
        glGenBuffers(1, &groupBoundariesSSBO);
        glGenBuffers(1, &totalGroupsSSBO);


     //   glGenBuffers(1, &renderDataSSBO);
     //   glGenBuffers(1, &meshCountersSSBO);
     //   glGenBuffers(1, &meshMetaSSBO);
     //   glGenBuffers(1, &visibleFlagsSSBO);
     //   glGenBuffers(1, &instanceSSBO);
     //   // Indirect draw commands
     //   glGenBuffers(1, &drawCmdSSBO);
     //   // licznik draw calls
     ///*   glGenBuffers(1, &drawCountSSBO);*/
     //   glGenBuffers(1, &meshDataSSBO);
     //   glGenBuffers(1, &materialSSBO);
     //   glGenBuffers(1, &lightsSSBO);

     //   glGenBuffers(1, &totalVisibleSSBO);
        uint32_t maxGroups = (maxRenderObjects + 255) / 256;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxGroups * 256 * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

         // Stały rozmiar: globalPrefix = zawsze 256 uint
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, globalPrefixSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        // totalGroups = 1 uint
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, totalGroupsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instanceBufferCapacity * sizeof(GPUInstanceData), nullptr, GL_DYNAMIC_DRAW);
       
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
        //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        //
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, totalVisibleSSBO);
        //glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        //totalVisibleMapped = (uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);


        glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderDataSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(RenderData), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // UBO — frame data (binding = 0)
        //glGenBuffers(1, &frameUBO);
        //glBindBuffer(GL_UNIFORM_BUFFER, frameUBO);
        //glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameUBO), nullptr, GL_DYNAMIC_DRAW);
        //glBindBufferBase(GL_UNIFORM_BUFFER, 0, frameUBO);

        // UBO — światła (binding = 0)
        glGenBuffers(1, &lightsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightsUBO), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightsUBO);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        InitHiZ(width, height);
    }

    void InitHiZ(int width, int height)
    {
        if (hizTexture) glDeleteTextures(1, &hizTexture);

        hizMipLevels = 1 + (int)std::floor(std::log2(std::max(width, height)));

        glGenTextures(1, &hizTexture);
        glBindTexture(GL_TEXTURE_2D, hizTexture);
        glTexStorage2D(GL_TEXTURE_2D, hizMipLevels, GL_R32F, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    uint32_t RegisterMesh(const MeshData& data)
    {
        GPUMeshData meshData;
        meshData.indexCount = (uint32_t)data.indices.size();
        meshData.firstIndex = (uint32_t)allIndices.size();
        meshData.baseVertex = (uint32_t)allVertices.size();
        meshData.padding = 0;

        // vertices
        allVertices.insert(allVertices.end(), data.vertices.begin(), data.vertices.end());

        // indices
        allIndices.insert(allIndices.end(), data.indices.begin(), data.indices.end());

        meshesData.push_back(meshData);
        spdlog::error("Zarejestrowano mesh");
        spdlog::info(allVertices.size());

        return (GLuint)meshesData.size() - 1; //meshID
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

    uint32_t RegisterLight(LightComponent* light, TransformComponent* transform)
    {
        auto it = lightRegistry.find(light);
        if (it != lightRegistry.end()) return it->second;

        GPULight g{};
        g.position = glm::vec4(transform->position, (float)light->type);
        if (glm::dot(light->direction, light->direction) < 0.0001f)
        {
            g.direction = glm::vec4(TransformHelper::getForward(*transform), 0.0f);
        }
        else
        {
            g.direction = glm::vec4(light->direction, 0.0f);
        }

        g.ambient = glm::vec4(light->isOn ? light->ambient : glm::vec3(0), 0);
        g.diffuse = glm::vec4(light->isOn ? light->diffuse : glm::vec3(0), 0);
        g.specular = glm::vec4(light->isOn ? light->specular : glm::vec3(0), 0);
        g.params1 = glm::vec4(light->constant, light->linear, light->quadratic, 0);
        g.params2 = glm::vec4(light->cutOff, light->outerCutOff, light->isOn ? 1.0f : 0.0f, 0);

        uint32_t id = (uint32_t)gpuLights.size();
        gpuLights.push_back(g);
        lightRegistry[light] = id;

        return id;
    }

    void UploadObjects(const std::vector<RenderData>& objects)
    {
        uint32_t n = (uint32_t)objects.size();
        ResizeIfNeeded((int)n);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderDataSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, n * sizeof(RenderData), objects.data());

        // visibleFlags — resize jeśli scena urosła
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleFlagsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, n * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        // sortValues ping A: wartości początkowe = 0,1,2,...,n-1
        // (scatter.comp przepisuje je jako indeksy obiektów)
        std::vector<uint32_t> identity(n);
        std::iota(identity.begin(), identity.end(), 0u);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortValuesA_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, n * sizeof(uint32_t), identity.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortValuesB_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, n * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortKeysA_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, n * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortKeysB_SSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, n * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);


        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    //void UploadObjects(const std::vector<RenderData>& objects)
    //{
    //    ResizeIfNeeded((int)objects.size());

    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleFlagsSSBO);
    //    glBufferData(GL_SHADER_STORAGE_BUFFER, objects.size() * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);


    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderDataSSBO);
    //    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, objects.size() * sizeof(RenderData), objects.data());
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    //}

    void UploadMeshes()
    {
        uint32_t meshCount = (uint32_t)meshesData.size();

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(Vertex), allVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndices.size() * sizeof(uint32_t), allIndices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshDataSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, meshesData.size() * sizeof(GPUMeshData), meshesData.data(), GL_STATIC_DRAW);

        // meshCounters — reset będzie co klatkę przez glClearNamedBufferData
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshCountersSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);


        // groupBoundaries — max tyle wpisów co obiektów (każdy mógłby być inną grupą)
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, groupBoundariesSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // drawCmdSSBO — max tyle komend co grup = max meshCount * materialCount,
        // ale bezpiecznym upper-boundem jest meshCount (jeden mesh = jedna komenda minimum)
        // Jeśli masz wiele materiałów per mesh, zwiększ tutaj.
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCmdSSBO);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, maxRenderObjects * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }

    //void UploadMeshes() {
    //    uint32_t meshCount = (uint32_t)meshesData.size();

    //    glBindVertexArray(VAO);
    //    // VBO
    //    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //    glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(Vertex), allVertices.data(), GL_STATIC_DRAW);

    //    // EBO
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //    glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndices.size() * sizeof(unsigned int), allIndices.data(), GL_STATIC_DRAW);
    //    glBindVertexArray(0);

    //    // MeshData SSBO
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshDataSSBO);
    //    glBufferData(GL_SHADER_STORAGE_BUFFER, meshesData.size() * sizeof(GPUMeshData), meshesData.data(), GL_STATIC_DRAW);

    //    //MeshCounters
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshCountersSSBO);
    //    glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
    //    //glNamedBufferData(meshCountersSSBO, meshCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

    //    // MeshMeta
    //    meshMetaCPU.resize(meshCount, { 0, 0, 0, 0 });
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshMetaSSBO);
    //    glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(GPUMeshMeta), meshMetaCPU.data(), GL_DYNAMIC_DRAW);

    //    // DrawCommands SSBO
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCmdSSBO);
    //    glBufferData(GL_SHADER_STORAGE_BUFFER, meshCount * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);

    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    //}

    void UploadMaterials()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(GPUMaterial), materials.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    size_t prevSizeLights = 0;

    void UpdateAndUploadLights(std::vector<LightComponent*>& lights, std::vector<TransformComponent*>& transforms)
    {
        if (lights.empty()) return;

        uint32_t count = std::min((uint32_t)lights.size(), (uint32_t)MAX_UBO_LIGHTS);
        gpuLights.resize(count);

        for (size_t i = 0; i < count; i++)
        {
            LightComponent* light = lights[i];
            TransformComponent* transform = transforms[i];
            if (!light || !transform) continue;

            GPULight& g = gpuLights[i];

            const bool on = light->isOn;

            g.position = glm::vec4(transform->position, (float)light->type);
            g.direction = (glm::length2(light->direction) < 0.0001f) ? glm::vec4(TransformHelper::getForward(*transform), 0.0f) : glm::vec4(light->direction, 0.0f);

            const glm::vec3& zero = glm::vec3(0.0f);
            g.ambient = glm::vec4(on ? light->ambient : zero, 0.0f);
            g.diffuse = glm::vec4(on ? light->diffuse : zero, 0.0f);
            g.specular = glm::vec4(on ? light->specular : zero, 0.0f);
            g.params1 = glm::vec4(light->constant, light->linear, light->quadratic, 0.0f);
            g.params2 = glm::vec4(light->cutOff, light->outerCutOff, on ? 1.0f : 0.0f, 0.0f);
        }


        glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(GPULight), gpuLights.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        //if (prevSizeLights == gpuLights.size())
        //{
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsSSBO);
        //    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuLights.size() * sizeof(GPULight), gpuLights.data());
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        //}
        //else
        //{
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsSSBO);
        //    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuLights.size() * sizeof(GPULight), gpuLights.data(), GL_DYNAMIC_DRAW);
        //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        //}

        prevSizeLights = count;
    }

    void UpdateLight(LightComponent* light, TransformComponent* transform)
    {
        auto it = lightRegistry.find(light);
        if (it == lightRegistry.end()) return;

        uint32_t id = it->second;
        GPULight& g = gpuLights[id];
        g.position = glm::vec4(transform->position, (float)light->type);

        if (glm::dot(light->direction, light->direction) < 0.0001f)
        {
            g.direction = glm::vec4(TransformHelper::getForward(*transform), 0.0f);
        }
        else
        {
            g.direction = glm::vec4(light->direction, 0.0f);
        }

        g.ambient = glm::vec4(light->isOn ? light->ambient : glm::vec3(0), 0);
        g.diffuse = glm::vec4(light->isOn ? light->diffuse : glm::vec3(0), 0);
        g.specular = glm::vec4(light->isOn ? light->specular : glm::vec3(0), 0);
        g.params1 = glm::vec4(light->constant, light->linear, light->quadratic, 0);
        g.params2 = glm::vec4(light->cutOff, light->outerCutOff, light->isOn ? 1.0f : 0.0f, 0);
    }

    void AllocateLightsBuffer()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, gpuLights.size() * sizeof(GPULight), gpuLights.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void UploadLights()
    {
        if (gpuLights.empty()) return;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuLights.size() * sizeof(GPULight), gpuLights.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }


    void ResetCounters()
    {
        uint32_t zero = 0;
        glClearNamedBufferData(meshCountersSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        glClearNamedBufferData(totalGroupsSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
    }


    void ResetMeshCounters()
    {
        uint32_t meshCount = (uint32_t)meshesData.size();
        // Jeden clear zamiast N subData — szybsze
        uint32_t zero = 0;
        glClearNamedBufferData(meshCountersSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); // nullptr = wypełnij zerami
    }

    // hiz_culling_count.comp
    //   binding 1 = renderDataSSBO
    //   binding 2 = meshCountersSSBO
    //   binding 3 = visibleFlagsSSBO
    //   binding 4 = sortKeysA_SSBO   (klucze do sortowania)
    void DispatchCountPass(const glm::mat4& viewProj, uint32_t objectCount)
    {

        shaderHizCullCount->use();

        glUniformMatrix4fv(glGetUniformLocation(shaderHizCullCount->ID, "viewProjection"), 1, GL_FALSE, &viewProj[0][0]);
        glUniform2f(glGetUniformLocation(shaderHizCullCount->ID, "screenSize"), (float)screenWidth, (float)screenHeight);
        glUniform1i(glGetUniformLocation(shaderHizCullCount->ID, "hizMipLevels"), hizMipLevels);
        glUniform1ui(glGetUniformLocation(shaderHizCullCount->ID, "objectCount"), objectCount);

        glBindTextureUnit(0, hizTexture);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, renderDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, meshCountersSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, visibleFlagsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sortKeysA_SSBO);

        glDispatchCompute((objectCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Krok 2a: histogram jednego bitu radix sort
    // histogram.comp
    //   binding 0 = keysSSBO (ping lub pong)
    //   binding 1 = histogramSSBO
    void DispatchHistogram(GLuint keysSSBO, uint32_t elementCount, uint32_t bitShift)
    {
        uint32_t groups = (elementCount + 255) / 256;
        lastHistogramGroups = groups;

        // Wyczyść histogram przed nowym passem
        uint32_t zero = 0;
        glClearNamedBufferData(histogramSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

        shaderHistogram->use();
        glUniform1ui(glGetUniformLocation(shaderHistogram->ID, "elementCount"), elementCount);
        glUniform1ui(glGetUniformLocation(shaderHistogram->ID, "bitShift"), bitShift);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, histogramSSBO);

        glDispatchCompute(groups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Krok 2c: scatter — przenosi elementy do posortowanych pozycji
    // scatter.comp
    //   binding 0 = keysIn
    //   binding 1 = valuesIn
    //   binding 2 = histogramSSBO
    //   binding 3 = globalPrefixSSBO
    //   binding 4 = keysOut
    //   binding 5 = valuesOut
    void DispatchScatter(GLuint keysIn, GLuint valuesIn, GLuint keysOut, GLuint valuesOut,
        uint32_t elementCount, uint32_t bitShift)
    {
        shaderScatter->use();
        glUniform1ui(glGetUniformLocation(shaderScatter->ID, "elementCount"), elementCount);
        glUniform1ui(glGetUniformLocation(shaderScatter->ID, "bitShift"), bitShift);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, valuesIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, histogramSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, globalPrefixSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, keysOut);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, valuesOut);

        glDispatchCompute((elementCount + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Krok 2: pełny 32-bitowy radix sort (4 passy × 8 bitów)
    // Po zakończeniu:  sortKeysA   = posortowane klucze
    //                 sortValuesA = posortowane indeksy obiektów
    void DispatchRadixSort(uint32_t elementCount)
    {
        // Pary ping-pong: A→B, B→A, A→B, B→A
        GLuint keys[2] = { sortKeysA_SSBO,   sortKeysB_SSBO };
        GLuint values[2] = { sortValuesA_SSBO, sortValuesB_SSBO };
        int ping = 0;

        for (int pass = 0; pass < 4; pass++) {
            uint32_t shift = pass * 8;
            int pong = 1 - ping;

            DispatchHistogram(keys[ping], elementCount, shift);
            DispatchGlobalPrefix();
            DispatchScatter(keys[ping], values[ping], keys[pong], values[pong], elementCount, shift);

            ping = pong;
        }
        // ping wrócił do 0 → wynik w sortKeysA_SSBO, sortValuesA_SSBO
    }


    // Krok 2b: exclusive prefix sum po bucketach (1 workgroup, 256 wątków)
    // global_prefix.comp
    //   binding 0 = histogramSSBO
    //   binding 1 = globalPrefixSSBO
    void DispatchGlobalPrefix()
    {
        shaderGlobalPrefix->use();
        glUniform1ui(glGetUniformLocation(shaderGlobalPrefix->ID, "groupCount"), lastHistogramGroups);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, globalPrefixSSBO);

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Krok 3: wyznacz granice grup (meshID, materialID) w posortowanej tablicy
    // prefix_sum.comp
    //   binding 0 = renderDataSSBO     (Objects)
    //   binding 1 = sortValuesA_SSBO   (SortedIndices)
    //   binding 2 = sortKeysA_SSBO     (SortedKeys)
    //   binding 3 = groupBoundariesSSBO(groupStart[])
    //   binding 4 = totalGroupsSSBO
    void DispatchGroupBoundaries(uint32_t visibleCount)
    {
        shaderPrefixSum->use();
        glUniform1ui(glGetUniformLocation(shaderPrefixSum->ID, "visibleCount"), visibleCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortValuesA_SSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sortKeysA_SSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, groupBoundariesSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, totalGroupsSSBO);

        glDispatchCompute((visibleCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Krok 4: przepisz instancje w posortowanej kolejności
    // write_pass.comp
    //   binding 0 = renderDataSSBO   (Objects)
    //   binding 1 = sortValuesA_SSBO (SortedIndices)
    //   binding 2 = instanceSSBO
    void DispatchWritePass(uint32_t visibleCount)
    {
        ResizeInstanceBufferIfNeeded(visibleCount);

        shaderWritePass->use();
        glUniform1ui(glGetUniformLocation(shaderWritePass->ID, "visibleCount"), visibleCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortValuesA_SSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instanceSSBO);

        glDispatchCompute((visibleCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    
    //void DispatchPrefixSum()
    //{
    //    shaderPrefixSum->use();

    //    uint32_t meshCount = (uint32_t)meshesData.size();
    //    glUniform1ui(glGetUniformLocation(shaderPrefixSum->ID, "meshCount"), meshCount);

    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshCountersSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, totalVisibleSSBO); // totalVisible

    //    glDispatchCompute((meshCount + 63) / 64, 1, 1);
    //    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

    //    ResizeInstanceBufferIfNeeded(*totalVisibleMapped);
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

    //void DispatchWritePass(const glm::mat4& viewProj, uint32_t objectCount)
    //{
    //    shaderHizWritePass->use();

    //    glUniform1ui(glGetUniformLocation(shaderHizWritePass->ID, "objectCount"), objectCount);

    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderDataSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instanceSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, visibleFlagsSSBO);

    //    glDispatchCompute((objectCount + 127) / 128, 1, 1);
    //    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    //}

    // Krok 5: zbuduj DrawIndirect commands — po jednej na grupę (meshID, materialID)
    // build_commands.comp
    //   binding 0 = meshDataSSBO
    //   binding 1 = sortKeysA_SSBO      (SortedKeys)
    //   binding 2 = groupBoundariesSSBO (groupStart[])
    //   binding 3 = totalGroupsSSBO
    //   binding 4 = drawCmdSSBO
    void DispatchBuildCommands(uint32_t visibleCount, uint32_t totalGroups)
    {
        if (totalGroups == 0) return;

        shaderBuildCmds->use();
        glUniform1ui(glGetUniformLocation(shaderBuildCmds->ID, "visibleCount"), visibleCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortKeysA_SSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, groupBoundariesSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, totalGroupsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, drawCmdSSBO);

        glDispatchCompute((totalGroups + 63) / 64, 1, 1);
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }

    //void DispatchBuildCommands()
    //{
    //    uint32_t meshCount = (uint32_t)meshesData.size();

    //    shaderBuildCmds->use();
    //    glUniform1ui(glGetUniformLocation(shaderBuildCmds->ID, "meshCount"), meshCount);

    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshDataSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawCmdSSBO);

    //    glDispatchCompute((meshCount + 63) / 64, 1, 1);
    //    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    //}

    void CopyDepthToHiZ(GLuint depthTexture)
    {
        glCopyImageSubData(depthTexture, GL_TEXTURE_2D, 0, 0, 0, 0, hizTexture, GL_TEXTURE_2D, 0, 0, 0, 0, screenWidth, screenHeight, 1);
    }

    void BuildHiZ()
    {
        shaderHizDownsample->use();

        int w = screenWidth, h = screenHeight;

        for (int mip = 1; mip < hizMipLevels; mip++) {
            w = std::max(1, w / 2);
            h = std::max(1, h / 2);

            // Poprzedni mip jako sampler (texture view na mip-1)
            // Ograniczamy BASE/MAX żeby sampler czytał tylko ten poziom
            glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, mip - 1);
            glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, mip - 1);
            glBindTextureUnit(0, hizTexture);

            // Aktualny mip jako image2D (zapis)
            glBindImageTexture(1, hizTexture, mip, GL_FALSE, 0,
                GL_WRITE_ONLY, GL_R32F);

            glDispatchCompute((w + 7) / 8, (h + 7) / 8, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
                | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        // Przywróć pełny zakres mipów
        glTextureParameteri(hizTexture, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(hizTexture, GL_TEXTURE_MAX_LEVEL, hizMipLevels - 1);
    }


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

    void BindForDraw() {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, instanceSSBO); // vertex shader
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, materialSSBO);   // fragment shader
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, lightsSSBO);   // fragment shader
    }

    //uint32_t ReadDrawCount() {
    //    uint32_t count = 0;
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
    //    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &count);
    //    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    //    return count;
    //}

    void Draw(uint32_t maxDrawCount)
    {
        BindForDraw();
        glBindVertexArray(VAO);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCmdSSBO);

        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)maxDrawCount, sizeof(DrawElementsIndirectCommand));

        glBindVertexArray(0);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    }

    /*void Draw()
    {
        BindForDraw();
        glBindVertexArray(VAO);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCmdSSBO);
        
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshesData.size(), sizeof(DrawElementsIndirectCommand));

        glBindVertexArray(0);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    }*/
    struct RenderDataCPU {
        glm::mat4 modelMatrix;
        glm::vec4 aabbMin;
        glm::vec4 aabbMax;
        uint32_t meshID;
        uint32_t materialID;
        uint32_t skeletonID;
        uint32_t padding;
    };
    void RenderFrame(const glm::mat4& viewProj, const std::vector<RenderData>& objects, GLuint depthTexturePrevFrame, glm::vec3 currentCameraPos)
    {
        uint32_t objCount = (uint32_t)objects.size();
        if (objCount == 0) return;

        // 0. Wyślij dane obiektów na GPU
        UploadObjects(objects);

        // 1. Zbuduj HiZ z depth poprzedniej klatki
        //CopyDepthToHiZ(depthTexturePrevFrame);
        //BuildHiZ();

        // 2. Zeruj liczniki
        ResetCounters();

        // 3. Culling + generuj sortKeys (hiz_culling_count)
        DispatchCountPass(viewProj, objCount);
       /* glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        std::vector<uint32_t> keysAfterCull(std::min(objCount, 10u));
        glGetNamedBufferSubData(sortKeysA_SSBO, 0,
            keysAfterCull.size() * sizeof(uint32_t), keysAfterCull.data());
        for (size_t i = 0; i < keysAfterCull.size(); i++)
            spdlog::info("KEY_AFTER_CULL[{}]: {:#010x}", i, keysAfterCull[i]);

        std::vector<uint32_t> flags(std::min(objCount, 10u));
        glGetNamedBufferSubData(visibleFlagsSSBO, 0,
            flags.size() * sizeof(uint32_t), flags.data());
        for (size_t i = 0; i < flags.size(); i++)
            spdlog::info("FLAG[{}]: {}", i, flags[i]);

        uint32_t meshCount = GetMeshCount();
        std::vector<uint32_t> counters(meshCount);
        glGetNamedBufferSubData(meshCountersSSBO, 0,
            meshCount * sizeof(uint32_t), counters.data());
        for (size_t i = 0; i < std::min(meshCount, 8u); i++)
            spdlog::info("COUNTER[{}]: {}", i, counters[i]);*/
        // 4. Radix sort po kluczu (meshID|materialID) — 4 passy
        //    Wynik: sortKeysA = klucze, sortValuesA = indeksy obiektów
        DispatchRadixSort(objCount);

        //glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        //std::vector<uint32_t> sortedKeys(std::min(objCount, 15u));
        //std::vector<uint32_t> sortedVals(std::min(objCount, 15u));
        //glGetNamedBufferSubData(sortKeysA_SSBO, 0,
        //    sortedKeys.size() * sizeof(uint32_t), sortedKeys.data());
        //glGetNamedBufferSubData(sortValuesA_SSBO, 0,
        //    sortedVals.size() * sizeof(uint32_t), sortedVals.data());
        //for (size_t i = 0; i < sortedKeys.size(); i++)
        //    spdlog::info("SORTED[{}]: key={:#010x} val={}", i, sortedKeys[i], sortedVals[i]);
        //// 5. Wyznacz granice grup (meshID, materialID)
        //    Na razie używamy objCount jako visibleCount — culling zakomentowany w shaderze
        //    Gdy culling zostanie włączony, przekaż tu rzeczywistą liczbę widocznych.
        uint32_t visibleCount = objCount;
        DispatchGroupBoundaries(visibleCount);

        // 6. Przepisz instancje w kolejności posortowanej (write_pass)
        DispatchWritePass(visibleCount);

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        uint32_t totalGroups = 0;
        glGetNamedBufferSubData(totalGroupsSSBO, 0, sizeof(uint32_t), &totalGroups);

        // 7. Zbuduj komendy DrawIndirect (build_commands)
        DispatchBuildCommands(visibleCount, totalGroups);

        // 8. Rysuj
        shaderRender->use();
        shaderRender->setMat4("viewProjection", viewProj);
        shaderRender->setVec3("viewPos", currentCameraPos);
        shaderRender->setInt("numLights", (int)gpuLights.size());

        //// 1. Sprawdź totalGroups
        //spdlog::info("totalGroups = {}, visibleCount = {}", totalGroups, visibleCount);

        //// 2. Readback drawCmdSSBO - co faktycznie idzie do GPU
        //std::vector<DrawElementsIndirectCommand> cmds(totalGroups);
        //glGetNamedBufferSubData(drawCmdSSBO, 0,
        //    totalGroups * sizeof(DrawElementsIndirectCommand), cmds.data());
        //for (uint32_t i = 0; i < std::min(totalGroups, 5u); i++) {
        //    spdlog::info("CMD[{}]: count={} instanceCount={} firstIndex={} baseVertex={} baseInstance={}",
        //        i, cmds[i].count, cmds[i].instanceCount,
        //        cmds[i].firstIndex, cmds[i].baseVertex, cmds[i].baseInstance);
        //}

        //// 3. Readback meshDataSSBO - co jest zarejestrowane
        //meshCount = GetMeshCount();
        //std::vector<GPUMeshData> meshes(meshCount);
        //glGetNamedBufferSubData(meshDataSSBO, 0,
        //    meshCount * sizeof(GPUMeshData), meshes.data());
        //for (uint32_t i = 0; i < std::min(meshCount, 5u); i++) {
        //    spdlog::info("MESH[{}]: indexCount={} firstIndex={} baseVertex={}",
        //        i, meshes[i].indexCount, meshes[i].firstIndex, meshes[i].baseVertex);
        //}

        //// 4. Readback instanceSSBO - pierwsze 3 macierze modeli
        //std::vector<GPUInstanceData> insts(std::min(visibleCount, 3u));
        //glGetNamedBufferSubData(instanceSSBO, 0,
        //    insts.size() * sizeof(GPUInstanceData), insts.data());
        //for (size_t i = 0; i < insts.size(); i++) {
        //    spdlog::info("INST[{}]: mat[0]={},{},{},{}", i,
        //        insts[i].model[0][0], insts[i].model[0][1],
        //        insts[i].model[0][2], insts[i].model[0][3]);
        //}

        //// 5. Readback sortKeysA - sprawdź czy sort działa
        //std::vector<uint32_t> keys(std::min(objCount, 10u));
        //glGetNamedBufferSubData(sortKeysA_SSBO, 0,
        //    keys.size() * sizeof(uint32_t), keys.data());
        //for (size_t i = 0; i < keys.size(); i++) {
        //    uint32_t meshID = keys[i] >> 16u;
        //    uint32_t matID = keys[i] & 0xFFFFu;
        //    spdlog::info("KEY[{}]: raw={:#010x} meshID={} matID={}", i, keys[i], meshID, matID);
        //}


        //std::vector<RenderDataCPU> rdBuf(std::min(objCount, 10u));
        //glGetNamedBufferSubData(renderDataSSBO, 0,
        //    rdBuf.size() * sizeof(RenderDataCPU), rdBuf.data());
        //for (size_t i = 0; i < rdBuf.size(); i++) {
        //    spdlog::info("RD[{}]: meshID={} matID={} model[3]={},{},{}",
        //        i, rdBuf[i].meshID, rdBuf[i].materialID,
        //        rdBuf[i].modelMatrix[3][0],  // pozycja X
        //        rdBuf[i].modelMatrix[3][1],  // pozycja Y
        //        rdBuf[i].modelMatrix[3][2]); // pozycja Z
        //}

        Draw(totalGroups);
    }


  //  void RenderFrame(const glm::mat4& viewProj, const std::vector<RenderData>& objects, GLuint depthTexturePrevFrame, glm::vec3 currentCameraPos)
  //  {
  //      uint32_t objCount = (uint32_t)objects.size();

  //      // 0. Aktualizuj obiekty na GPU
  //      UploadObjects(objects);
  //      //UploadLights();

  //      // 1. Zbuduj HiZ z depth poprzedniej klatki
  ///*      CopyDepthToHiZ(depthTexturePrevFrame);
  //      BuildHiZ();*/

  //      // 2. Zeruj liczniki meshów
  //      ResetMeshCounters();

  //      // 3. COUNT PASS: culling + zliczanie (bez zapisu instancji)
  //      DispatchCountPass(viewProj, objCount);
  //      // barrier wewnątrz DispatchCountPass

  //      // 4. PREFIX-SUM na CPU (czyta tylko meshCount uint32!)
  //      DispatchPrefixSum();
  //      // wewnątrz: glGetNamedBufferSubData + wysyłka MeshMeta + ewentualny resize

  //      // 5. WRITE PASS: zapis instancji
  //      DispatchWritePass(viewProj, objCount);
  //      // barrier wewnątrz DispatchWritePass

  //      // 6. BUILD COMMANDS: DrawIndirect per mesh
  //      DispatchBuildCommands();
  //      // barrier wewnątrz DispatchBuildCommands

  //      shaderRender->use();
  //      shaderRender->setMat4("viewProjection", viewProj);
  //      //shaderRender->setBool("isAnimated", false);
  //      shaderRender->setVec3("viewPos", currentCameraPos);
  //      shaderRender->setInt("numLights", (int)gpuLights.size());

  //      // 7. Rysuj
  //      Draw();
  //  }

    uint32_t GetMeshCount()    const { return (uint32_t)meshesData.size(); }
    uint32_t GetMaterialCount()const { return (uint32_t)materials.size(); }
    int      GetHiZMipLevels() const { return hizMipLevels; }
};

#endif

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