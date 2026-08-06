#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <glad/glad.h>
#include <string>
#include <unordered_map>
//#include <spdlog/spdlog.h>
#include <assimp/texture.h>
#include <memory>
#include "model.h"

class Model;

// Format file DDS
#pragma pack(push, 1)
struct DDS_PIXELFORMAT
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct DDS_HEADER
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};
#pragma pack(pop)

static uint32_t createFourCC(char a, char b, char c, char d)
{
    return (uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);
}

class ResourceManager
{
public:
    static std::unordered_map<std::string, TextureData> Textures;

    static std::unordered_map<std::string, std::shared_ptr<Model>> Models;
    
    static std::unordered_map<GLuint, std::string> TextureIDToPath;
    
    static std::unordered_map<uint32_t, std::string> MeshNodeToModelPath;

    static void RegisterMeshNodes(ModelNode* node, const std::string& path);

    static std::string GetModelPathByMeshNodeID(uint32_t id);

    static std::string GetTexturePath(GLuint id);

    static TextureData LoadTexture(const std::string& path, const std::string& directory = "", const aiTexture* aiTex = nullptr);

    static std::shared_ptr<Model> LoadModel(const std::string& path);

    static void Clear();

    static TextureData CreateTextureFromColor(const std::string& name, const glm::vec3& color);

    static void SaveAsset();

    static void LoadAssets(std::string& path);

private:
    static TextureData loadTextureFromFile(const std::string& fullPath, const std::string& path, const std::string& directory, const aiTexture* aiTex);

};

#endif