#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <assimp/texture.h>
#include <memory>

class Model;

class ResourceManager
{
public:
    static std::unordered_map<std::string, std::weak_ptr<unsigned int>> Textures;

    static std::unordered_map<std::string, std::weak_ptr<Model>> Models;

    static std::shared_ptr<unsigned int> LoadTexture(const std::string& path, const std::string& directory = "", const aiTexture* aiTex = nullptr);

    static std::shared_ptr<Model> LoadModel(const std::string& path);

    static void Clear();

private:
    static unsigned int loadTextureFromFile(const std::string& path, const std::string& directory, const aiTexture* aiTex);
};

#endif