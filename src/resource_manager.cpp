#include "resource_manager.h"
#include <stb_image.h>
#include <iostream>
#include "model.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

std::unordered_map<std::string, std::weak_ptr<unsigned int>> ResourceManager::Textures;

std::shared_ptr<unsigned int> ResourceManager::LoadTexture(const std::string& path, const std::string& directory, const aiTexture* aiTex)
{
    std::string fullPath = path;
    if (!directory.empty() && !aiTex) {
        fullPath = directory + '/' + path;
    }
    else if (aiTex) {
        fullPath = "embedded_" + path;
    }

    auto it = Textures.find(fullPath);
    if (it != Textures.end())
    {
        // czy tekstura nadal zyje w VRAM
        if (std::shared_ptr<unsigned int> sharedTex = it->second.lock())
        {
            return sharedTex;
        }
    }

    // Jesli nie ma, ladujemy z dysku
    unsigned int rawTextureID = loadTextureFromFile(path, directory, aiTex);

    // gdy wskaznik ginie, automatyczine wykonuje funkcje glDeleteTextures
    std::shared_ptr<unsigned int> textureID(new unsigned int(rawTextureID), [](unsigned int* ptr) {
        glDeleteTextures(1, ptr);
        delete ptr;
        spdlog::info("Zwolniono pamiec VRAM po nieuzywanej teksturze!");
        });

    if (rawTextureID != 0) {
        Textures[fullPath] = textureID;
    }

    return textureID;
}

std::unordered_map<std::string, std::weak_ptr<Model>> ResourceManager::Models;

std::shared_ptr<Model> ResourceManager::LoadModel(const std::string& path)
{
    auto it = Models.find(path);
    if (it != Models.end())
    {
        if (std::shared_ptr<Model> sharedModel = it->second.lock())
        {
            return sharedModel;
        }
    }

    std::shared_ptr<Model> model = std::make_shared<Model>(path);

    Models[path] = model;

    spdlog::info("ResourceManager: Zaladowano model {}", path);
    return model;
}

unsigned int ResourceManager::loadTextureFromFile(const std::string& path, const std::string& directory, const aiTexture* aiTex)
{
    unsigned int textureID = 0;

    int width, height, nrComponents;
    unsigned char* data = nullptr;

    if (aiTex)
    {
        if (aiTex->mHeight == 0)
        {
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(aiTex->pcData), aiTex->mWidth, &width, &height, &nrComponents, 0);
        }
        else
        {
            width = aiTex->mWidth;
            height = aiTex->mHeight;
            nrComponents = 4;
            data = new unsigned char[width * height * 4];
            memcpy(data, aiTex->pcData, width * height * 4);
        }
    }
    else
    {
        std::string filename = directory.empty() ? path : directory + '/' + path;
        data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    }

    if (data)
    {
        glGenTextures(1, &textureID);

        GLenum format = GL_RED;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLfloat maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
        //

        if (aiTex && aiTex->mHeight != 0)
            delete[] data;
        else
            stbi_image_free(data);

        spdlog::info("ResourceManager: Zaladowano teksture {}", path);
    }
    else
    {
        spdlog::error("ResourceManager: BLAD ladowania tekstury {}", path);
        if (data) stbi_image_free(data);
        return 0;
    }

    return textureID;
}

void ResourceManager::Clear()
{
    Textures.clear();
    Models.clear();
    spdlog::info("ResourceManager: Wyczyszczono pamiec tekstur.");
}