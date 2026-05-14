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
    static std::unordered_map<std::string, GLuint> Textures;

    static std::unordered_map<std::string, std::shared_ptr<Model>> Models;

    static GLuint LoadTexture(const std::string& path, const std::string& directory = "", const aiTexture* aiTex = nullptr);

    static std::shared_ptr<Model> LoadModel(const std::string& path);

    static void Clear();

    static GLuint CreateTextureFromColor(const std::string& name, const glm::vec3& color);

private:
    static unsigned int loadTextureFromFile(const std::string& path, const std::string& directory, const aiTexture* aiTex);

};

#endif