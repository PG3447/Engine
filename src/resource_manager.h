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

struct LoadedTexture
{
    GLuint id = 0;
    bool hasAlpha = false;
};

class ResourceManager
{
public:
    static std::unordered_map<std::string, GLuint> Textures;

    static std::unordered_map<std::string, std::weak_ptr<Model>> Models;

    static LoadedTexture LoadTexture(const std::string& path, const std::string& directory = "", const aiTexture* aiTex = nullptr);

    static std::shared_ptr<ModelNode> LoadModel(const std::string& path);

    static void Clear();

    static GLuint CreateTextureFromColor(const std::string& name, const glm::vec3& color);

private:
    static LoadedTexture loadTextureFromFile(const std::string& path, const std::string& directory, const aiTexture* aiTex);

};

#endif