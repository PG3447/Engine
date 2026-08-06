#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <assimp/texture.h>
#include <memory>
#include "model.h"

class Model;


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