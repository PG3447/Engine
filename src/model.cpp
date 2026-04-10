#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "model.h"
#include "mesh.h"
#include "shader.h"

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

#include "resource_manager.h"

Model::Model() : gammaCorrection(false)
{
}

Model::~Model() {
    if (instanceVBO != 0) {
        glDeleteBuffers(1, &instanceVBO);
    }
}

Model::Model(string const& path, bool gamma) : gammaCorrection(gamma)
{
    loadModel(path);
}

/*
// constructor, expects a filepath to a 3D model.
Model::Model(string const& path, float meshScale, bool gamma) : meshScale(meshScale), gammaCorrection(gamma)
{
    if (!path.empty())
    {
        loadModel(path);
    }
}
*/

void Model::PrepareInstancing()
{
    if (instancingPrepared)
        return;

    for (auto& mesh : meshes)
        mesh.EnableInstancing();

    instancingPrepared = true;
}


// draws the model, and thus all its meshes
void Model::Draw(Shader& shader, GLsizei instanceCount)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        meshes[i].Draw(shader, instanceCount);
    }
}
void Model::turnOnReflect(unsigned int cubemapTexture)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        meshes[i].reflect = true;
        meshes[i].cubemapTexture = cubemapTexture;
    }
}

void Model::loadModel(string const& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes
    ); 
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        spdlog::error("ERROR::ASSIMP:: {}", importer.GetErrorString());
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    name = path;

    // Pobieramy drzewo z korzenia
    Model rootNode = processNode(scene->mRootNode, scene);
    this->meshes = std::move(rootNode.meshes);
    this->children = std::move(rootNode.children);
    this->transform = rootNode.transform;
}

Model Model::processNode(aiNode* node, const aiScene* scene)
{
    Model model;
    aiVector3D scale, pos;
    aiQuaternion rot;
    node->mTransformation.Decompose(scale, rot, pos);

    model.name = node->mName.C_Str();
    model.directory = this->directory;

    model.transform.setLocalPosition({ pos.x, pos.y, pos.z });
    model.transform.setLocalRotation({ glm::degrees(rot.x), glm::degrees(rot.y), glm::degrees(rot.z) });
    model.transform.setLocalScale({ scale.x, scale.y, scale.z });

    model.meshes.reserve(node->mNumMeshes);
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(processMesh(mesh, scene));
    }

    model.children.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        model.children.push_back(processNode(node->mChildren[i], scene));
    }

    return model;
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        // normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            // tangent
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
            // bitangent
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", scene);
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices, textures);
}


// checks all material textures of a given type and loads the textures if they're not loaded yet.
// the required info is returned as a Texture struct.
vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName, const aiScene* scene)
{
    vector<Texture> textures;
    textures.reserve(mat->GetTextureCount(type));
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        Texture texture;

        if (str.C_Str()[0] == '*' && scene) // embedded texture
        {
            int texIndex = atoi(str.C_Str() + 1);
            aiTexture* aiTex = scene->mTextures[texIndex];
            // U¿ywamy Resource Managera!
            texture.id = ResourceManager::LoadTexture(str.C_Str(), "", aiTex);
        }
        else // normalna tekstura z pliku
        {
            // U¿ywamy Resource Managera!
            texture.id = ResourceManager::LoadTexture(str.C_Str(), this->directory);
        }

        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
    }
    return textures;
}


std::unique_ptr<Model> Model::createOrbit(float radius, int segments, float tiltDegrees, float scale, vector<Texture>* textures)
{
    auto orbit = std::make_unique<Model>();
    orbit->transform.setLocalScale(glm::vec3(scale));

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float tilt = glm::radians(tiltDegrees);

    for (int i = 0; i <= segments; ++i)
    {
        float angle = (float)i / segments * 2.0f * 3.14159265f;

        float x = radius * cos(angle);
        float y = 0.0f;
        float z = radius * sin(angle);

        glm::vec3 p(
            x,
            y * cos(tilt) - z * sin(tilt),
            y * sin(tilt) + z * cos(tilt)
        );

        Vertex v{};
        v.Position = p;
        v.Normal = glm::vec3(0, 1, 0);
        v.TexCoords = glm::vec2((float)i / segments, 0.0f);

        vertices.push_back(v);
        indices.push_back(i);
    }
    vector<Texture> finalTextures;

    if (textures != nullptr)
        finalTextures = *textures;

    Mesh orbitMesh(vertices, indices, finalTextures);
    orbitMesh.meshType = Mesh::MESH_LINES;

    orbit->meshes.push_back(std::move(orbitMesh));
    return orbit;
}

std::unique_ptr<Model> Model::createSphere(int rings, int sectors, const std::string& texturePath)
{
    auto model = std::make_unique<Model>();

    std::vector<float> ringIDs;
    for (int r = 0; r < rings; ++r)
        ringIDs.push_back((float)r);
    Mesh mesh(ringIDs);
    mesh.meshType = Mesh::MESH_POINTS;
    if (!texturePath.empty())
    {
        size_t pos = texturePath.find_last_of("/\\");
        std::string directory = "";
        std::string filename = texturePath;
        if (pos != std::string::npos)
        {
            directory = texturePath.substr(0, pos);
            filename = texturePath.substr(pos + 1);
        }

        Texture tex;
        tex.id = ResourceManager::LoadTexture(filename, directory);
        tex.type = "texture_diffuse";
        tex.path = texturePath;
        mesh.textures.push_back(tex);
    }
    model->meshes.push_back(mesh);

    return model;
}