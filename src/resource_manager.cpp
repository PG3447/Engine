#include "resource_manager.h"
#include <stb_image.h>
#include <iostream>


#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT  0x83F0
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

std::unordered_map<std::string, TextureData> ResourceManager::Textures;
std::unordered_map<std::string, std::shared_ptr<Model>> ResourceManager::Models;

std::unordered_map<GLuint, std::string> ResourceManager::TextureIDToPath;
std::unordered_map<uint32_t, std::string> ResourceManager::MeshNodeToModelPath;

void ResourceManager::RegisterMeshNodes(ModelNode* node, const std::string& path)
{
    if (!node) return;
    for (auto& mesh : node->meshes)
        MeshNodeToModelPath[mesh.meshNodeID] = path;
    for (auto& child : node->children)
        RegisterMeshNodes(child.get(), path);
}

std::string ResourceManager::GetModelPathByMeshNodeID(uint32_t id)
{
    auto it = MeshNodeToModelPath.find(id);
    return it != MeshNodeToModelPath.end() ? it->second : "";
}

std::string ResourceManager::GetTexturePath(GLuint id)
{
    if (id == 0) return "";
    auto it = TextureIDToPath.find(id);
    return it != TextureIDToPath.end() ? it->second : "";
}

TextureData ResourceManager::LoadTexture(const std::string& path, const std::string& directory, const aiTexture* aiTex)
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
        return it->second;
    }

    TextureData textureData = loadTextureFromFile(fullPath, path, directory, aiTex);

    if (textureData.id != 0) {
        Textures[fullPath].id = textureData.id;
        Textures[fullPath].hasAlpha = textureData.hasAlpha;
        TextureIDToPath[textureData.id] = fullPath;
    }

    return textureData;
}

std::shared_ptr<Model> ResourceManager::LoadModel(const std::string& path)
{
    auto it = Models.find(path);
    if (it != Models.end())
    {
        if (std::shared_ptr<Model> sharedModel = it->second)
        {
            return sharedModel;
        }
    }

    std::shared_ptr<Model> model = std::make_shared<Model>(path);

    if (!model->rootNode)
    {
        spdlog::error("Model load failed (rootNode null): {}", path);
        return nullptr;
    }

    Models[path] = model;
    RegisterMeshNodes(model->rootNode.get(), path);
    spdlog::info("ResourceManager: Zaladowano model {}", path);
    return model;
}


uint32_t fnv1aHash(const char* string)
{
    uint64_t hash = 0xcbf29ce484222325ULL; // offset
    while (*string)
    {
        hash ^= (unsigned char)(*string ++);
        hash *= 0x100000001b3ULL;
    }

    return hash;
}

inline static size_t TotalTextureMemory = 0;

TextureData ResourceManager::loadTextureFromFile(const std::string& fullPath, const std::string& path, const std::string& directory, const aiTexture* aiTex)
{
    unsigned int textureID = 0;

    int width, height, nrComponents;
    unsigned char* data = nullptr;
    bool hasAlpha = false;
    TextureData dataTexture = { textureID, hasAlpha };

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
        GLenum internalFormat = GL_COMPRESSED_RED;// GL_COMPRESSED_RED_RGTC1; // GL_RED
        if (nrComponents == 1)
        {
            format = GL_RED;
            internalFormat = GL_COMPRESSED_RED;// GL_COMPRESSED_RED_RGTC1; // GL_RED;
        }
        else if (nrComponents == 2)
        {
            format = GL_RG;
            internalFormat = GL_COMPRESSED_RG;// GL_COMPRESSED_RG_RGTC2; // GL_RG;
        }
        else if (nrComponents == 3)
        {
            format = GL_RGB;
            internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;//GL_COMPRESSED_RGB;// GL_COMPRESSED_RGB_S3TC_DXT1_EXT; // GL_RGB
        }
        else if (nrComponents == 4) {
            hasAlpha = true;
            format = GL_RGBA;
            internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;// GL_COMPRESSED_RGBA;// GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; // GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);


        uint32_t hashPath = fnv1aHash(fullPath.c_str());
        std::string cachePath = "res/texturesCache/" + std::to_string(hashPath) + ".dds";

        FILE* fileCache = fopen(cachePath.c_str(), "rb");

        if (fileCache)
        {
            uint32_t dwMagic;
            fread(&dwMagic, sizeof(dwMagic), 1, fileCache);
            
            if (dwMagic == createFourCC('D', 'D', 'S', ' '))
            {
                DDS_HEADER headerDDSCache{};

                fread(&headerDDSCache, sizeof(headerDDSCache), 1, fileCache);

                GLenum cachedInternalFormat = (headerDDSCache.ddspf.dwFourCC == createFourCC('D', 'X', 'T', '5')) ? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;

                uint32_t blockSize;

                if (cachedInternalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
                {
                    blockSize = 8;
                }
                else
                {
                    blockSize = 16;
                }

                uint32_t widthMip = headerDDSCache.dwWidth;
                uint32_t heightMip = headerDDSCache.dwHeight;


                for (uint32_t level = 0; level < headerDDSCache.dwMipMapCount; level++)
                {    
                    uint32_t dataSize = std::max(1u, (widthMip + 3) / 4) * std::max(1u, (heightMip + 3) / 4) * blockSize;
                    
                    std::vector<uint8_t> bufforCompressedData(dataSize);
                    fread(bufforCompressedData.data(), 1, dataSize, fileCache);
                    glCompressedTexImage2D(GL_TEXTURE_2D, level, cachedInternalFormat, widthMip, heightMip, 0, dataSize, bufforCompressedData.data());
                    
                    widthMip = std::max(1u, widthMip / 2);
                    heightMip = std::max(1u, heightMip / 2);
                }

                fclose(fileCache);
            }

        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);


            GLint numLeves = 1 + (GLint)std::floor(std::log2((float)std::max(width, height)));

            std::vector<std::vector<uint8_t>> mipLevels;
            for (GLint level = 0; level < numLeves; level++)
            {
                GLint mipWidth = 0;
                GLint mipHeight = 0;
                GLint mipCompressedSize = 0;

                glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &mipWidth);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &mipHeight);
                if (mipWidth == 0 || mipHeight == 0) break;

                glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &mipCompressedSize);
                
                std::vector<uint8_t> mipBuffor(mipCompressedSize);
                glGetCompressedTexImage(GL_TEXTURE_2D, level, mipBuffor.data());
                mipLevels.push_back(std::move(mipBuffor));
            }

            FILE* fileCacheSave = fopen(cachePath.c_str(), "wb");
            if (fileCacheSave)
            {
                uint32_t dwMagic = createFourCC('D', 'D', 'S', ' ');
                DDS_HEADER headerDDS{};
                headerDDS.dwSize = 124;
                headerDDS.dwFlags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x20000 | 0x80000;
                headerDDS.dwHeight = height;
                headerDDS.dwWidth = width;
                headerDDS.dwPitchOrLinearSize = (uint32_t)mipLevels[0].size();
                headerDDS.dwMipMapCount = (uint32_t)mipLevels.size();
                headerDDS.ddspf.dwSize = 32;
                headerDDS.ddspf.dwFlags = 0x4;
                headerDDS.ddspf.dwFourCC = (nrComponents == 4) ? createFourCC('D', 'X', 'T', '5') : createFourCC('D', 'X', 'T', '1');
                headerDDS.dwCaps = 0x1000 | 0x8 | 0x400000;

                fwrite(&dwMagic, sizeof(dwMagic), 1, fileCacheSave);
                fwrite(&headerDDS, sizeof(headerDDS), 1, fileCacheSave);
                for (auto& level : mipLevels)
                    fwrite(level.data(), 1, level.size(), fileCacheSave);
                fclose(fileCacheSave);
            }
        }


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

        size_t textureBytes = width * height * nrComponents;
        size_t textureBytesWithMipmaps =
            static_cast<size_t>(textureBytes * 1.333333f);

        spdlog::info(
            "Texture {}: {}x{} {}ch -> {:.2f} MB ({:.2f} MB with mipmaps)",
            path,
            width,
            height,
            nrComponents,
            textureBytes / (1024.0 * 1024.0),
            textureBytesWithMipmaps / (1024.0 * 1024.0)
        );

        TotalTextureMemory += textureBytesWithMipmaps;

        spdlog::info(
            "Total texture memory: {:.2f} MB",
            TotalTextureMemory / (1024.0 * 1024.0)
        );
    }
    else
    {
        spdlog::error("ResourceManager: BLAD ladowania tekstury {}", path);
        if (data) stbi_image_free(data);
        return dataTexture;
    }
    dataTexture = { textureID, hasAlpha };

    return dataTexture;
}

TextureData ResourceManager::CreateTextureFromColor(const std::string& name, const glm::vec3& color)
{
    auto it = Textures.find(name);
    if (it != Textures.end())
        return it->second;

    unsigned char data[3] = {
        (unsigned char)(glm::clamp(color.r, 0.0f, 1.0f) * 255),
        (unsigned char)(glm::clamp(color.g, 0.0f, 1.0f) * 255),
        (unsigned char)(glm::clamp(color.b, 0.0f, 1.0f) * 255)
    };

    GLuint tex = 0;
    glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);

    TextureData texture{ tex, false };
    Textures[name] = texture;

    spdlog::info("ResourceManager: Created color texture {}", name);

    return texture;
}

/*
#include <functional>

std::string id = relPath.string();
size_t hashID = std::hash<std::string>{}(id);
*/

void ResourceManager::SaveAsset()
{
    YamlConfig cfg;

    YAML::Node assetsNode;

    int i = 0;

    for (auto& [modelPath, model] : ResourceManager::Models)
    {
        if (!model)
            continue;

        YAML::Node modelNode;
        modelNode["path"] = modelPath;
        
        assetsNode["Assets"]["Models"][i++] = modelNode;
    }

    cfg.getRoot() = assetsNode;
    cfg.save("res/Yaml/assets.yaml");
}

void ResourceManager::LoadAssets(std::string& path)
{
    YamlConfig cfg;

    cfg.load(path);

    YAML::Node root = cfg.getRoot();

    if (!root["Assets"]["Models"])
        return;

    for (auto node : root["Assets"]["Models"])
    {
        std::string modelPath = node["path"].as<std::string>();

        ResourceManager::LoadModel(modelPath);
    }
}

void ResourceManager::Clear()
{
    Textures.clear();
    Models.clear();
    MeshNodeToModelPath.clear();
    spdlog::info("ResourceManager: Wyczyszczono pamiec tekstur.");
}