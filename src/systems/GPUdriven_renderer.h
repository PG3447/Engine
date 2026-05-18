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
    uint32_t padding;
    uint32_t padding2;
    glm::vec4 diffuseColorAndShininess;
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
    glm::vec2 padding;
};


class GPUDrivenRenderer {
private:
    //rejestr geometria
    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;

    //rejestr
    std::vector<GPUMeshData> meshesData;
    std::vector<GPUMaterial> materials;
    std::vector<GPUMeshMeta> meshMetaCPU;

    std::unordered_map<Material*, uint32_t> materialRegistry;
    std::unordered_map<GLuint, GLuint64> handleCacheTextures;

    uint32_t instanceBufferCapacity = 64;
    int maxRenderObjects = 64;

    GLuint64 GetOrCreateHandle(GLuint texID) {
        if (texID == 0) return 0;
        auto it = handleCacheTextures.find(texID);
        if (it != handleCacheTextures.end()) return it->second;

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
    GLuint visibleFlagsSSBO = 0; // bind 2
    GLuint totalVisibleSSBO = 0; // bind 2
    //build command
    GLuint instanceSSBO = 0; // bind 3
    GLuint drawCmdSSBO = 0; // bind 4
    GLuint drawCountSSBO = 0; // bind 5
    GLuint meshDataSSBO = 0; // bind 6
    GLuint materialSSBO = 0; /// bind 7

    uint32_t* totalVisibleMapped = nullptr;

    GLuint hizTexture = 0;
    int hizMipLevels = 0;
    int screenWidth = 0, screenHeight = 0;

    ComputeShader* shaderHizCullCount = nullptr;
    ComputeShader* shaderPrefixSum = nullptr;
    ComputeShader* shaderHizWritePass = nullptr;
    ComputeShader* shaderBuildCmds = nullptr;
    ComputeShader* shaderHizDownsample = nullptr;
    Shader* shaderRender = nullptr;


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
        glDeleteBuffers(1, &visibleFlagsSSBO);
        glDeleteBuffers(1, &totalVisibleSSBO);
        glDeleteBuffers(1, &instanceSSBO);
        glDeleteBuffers(1, &drawCmdSSBO);
        glDeleteBuffers(1, &drawCountSSBO);
        glDeleteBuffers(1, &meshDataSSBO);
        glDeleteBuffers(1, &materialSSBO);
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
        glGenBuffers(1, &visibleFlagsSSBO);
        glGenBuffers(1, &instanceSSBO);
        // Indirect draw commands
        glGenBuffers(1, &drawCmdSSBO);
        // licznik draw calls
        glGenBuffers(1, &drawCountSSBO);
        glGenBuffers(1, &meshDataSSBO);
        glGenBuffers(1, &materialSSBO);
        glGenBuffers(1, &totalVisibleSSBO);


        glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instanceBufferCapacity * sizeof(GPUInstanceData), nullptr, GL_DYNAMIC_DRAW);
       
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, totalVisibleSSBO);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        totalVisibleMapped = (uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);


        glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderDataSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, maxRenderObjects * sizeof(RenderData), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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
        gpu.padding = 0;
        gpu.padding2 = 0;
        gpu.diffuseColorAndShininess = glm::vec4(mat->diffuseColor, mat->shininess);

        uint32_t id = (uint32_t)materials.size();
        materials.push_back(gpu);
        materialRegistry[mat] = id;

        spdlog::error("Zarejestrowano material");
        spdlog::info(materialRegistry.size());
        return id; //materialID
    }


    void UploadObjects(const std::vector<RenderData>& objects)
    {
        ResizeIfNeeded((int)objects.size());

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleFlagsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, objects.size() * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);


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
        glBufferData(GL_SHADER_STORAGE_BUFFER, meshesData.size() * sizeof(GPUMeshData), meshesData.data(), GL_STATIC_DRAW);

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
    }

    void UploadMaterials()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(GPUMaterial), materials.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ResetMeshCounters()
    {
        uint32_t meshCount = (uint32_t)meshesData.size();
        // Jeden clear zamiast N subData — szybsze
        uint32_t zero = 0;
        glClearNamedBufferData(meshCountersSSBO, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero); // nullptr = wypełnij zerami
    }

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

        glDispatchCompute((objectCount + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    
    void DispatchPrefixSum()
    {
        shaderPrefixSum->use();

        uint32_t meshCount = (uint32_t)meshesData.size();
        glUniform1ui(glGetUniformLocation(shaderPrefixSum->ID, "meshCount"), meshCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshCountersSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, totalVisibleSSBO); // totalVisible

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

        ResizeInstanceBufferIfNeeded(*totalVisibleMapped);
    }

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

    void DispatchWritePass(const glm::mat4& viewProj, uint32_t objectCount)
    {
        shaderHizWritePass->use();

        glUniform1ui(glGetUniformLocation(shaderHizWritePass->ID, "objectCount"), objectCount);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderDataSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshMetaSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instanceSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, visibleFlagsSSBO);

        glDispatchCompute((objectCount + 63) / 64, 1, 1);
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


    void ResetDrawCount() {
        uint32_t zero = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    //void BindForCompute() {
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, renderDataSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshDataSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawCmdSSBO);
    //    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, drawCountSSBO);
    //}

    void BindForDraw() {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, instanceSSBO); // vertex shader
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, materialSSBO);   // fragment shader
    }

    uint32_t ReadDrawCount() {
        uint32_t count = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountSSBO);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &count);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return count;
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

    void RenderFrame(const glm::mat4& viewProj, const std::vector<RenderData>& objects, GLuint depthTexturePrevFrame, glm::vec3 currentCameraPos)
    {
        uint32_t objCount = (uint32_t)objects.size();

        // 0. Aktualizuj obiekty na GPU
        UploadObjects(objects);

        // 1. Zbuduj HiZ z depth poprzedniej klatki
  /*      CopyDepthToHiZ(depthTexturePrevFrame);
        BuildHiZ();*/

        // 2. Zeruj liczniki meshów
        ResetMeshCounters();

        // 3. COUNT PASS: culling + zliczanie (bez zapisu instancji)
        DispatchCountPass(viewProj, objCount);
        // barrier wewnątrz DispatchCountPass

        // 4. PREFIX-SUM na CPU (czyta tylko meshCount uint32!)
        DispatchPrefixSum();
        // wewnątrz: glGetNamedBufferSubData + wysyłka MeshMeta + ewentualny resize

        // 5. WRITE PASS: ten sam culling + zapis instancji
        DispatchWritePass(viewProj, objCount);
        // barrier wewnątrz DispatchWritePass

        // 6. BUILD COMMANDS: DrawIndirect per mesh
        DispatchBuildCommands();
        // barrier wewnątrz DispatchBuildCommands

        shaderRender->use();
        shaderRender->setMat4("viewProjection", viewProj);
        shaderRender->setVec3("viewPos", currentCameraPos);
        shaderRender->setBool("isAnimated", false);

        // 7. Rysuj
        Draw();
    }



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