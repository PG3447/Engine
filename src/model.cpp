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

    if (instanceVBO == 0) {
        glGenBuffers(1, &instanceVBO);
    }

    for (auto& mesh : meshes) {
        mesh.EnableInstancing(instanceVBO);
    }

    instancingPrepared = true;
}


// draws the model, and thus all its meshes
void Model::Draw(GLsizei instanceCount, Material* materialOverride)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        meshes[i].Draw(instanceCount, materialOverride);
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
    glm::quat q(rot.w, rot.x, rot.y, rot.z);

    glm::vec3 euler = glm::eulerAngles(q); // radiany
    euler = glm::degrees(euler);

    model.transform.setLocalRotation(euler);
    //model.transform.setLocalRotation({ glm::degrees(rot.x), glm::degrees(rot.y), glm::degrees(rot.z) });
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
    aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
    std::shared_ptr<Material> myMaterial = std::make_shared<Material>();

    // diffuse
    vector<Texture> diffuseMaps = loadMaterialTextures(aiMat, aiTextureType_DIFFUSE, "texture_diffuse", scene);
    if (diffuseMaps.empty()) {
        diffuseMaps = loadMaterialTextures(aiMat, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
    }
    if (!diffuseMaps.empty()) {
        myMaterial->diffuseMap = diffuseMaps[0].id;
    }
    else {
        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &color) == AI_SUCCESS ||
            aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
        {
            myMaterial->diffuseColor = glm::vec3(color.r, color.g, color.b);
        }
    }

    // specular
    vector<Texture> specularMaps = loadMaterialTextures(aiMat, aiTextureType_SPECULAR, "texture_specular", scene);
    if (!specularMaps.empty()) myMaterial->specularMap = specularMaps[0].id;

    // normal
    vector<Texture> normalMaps = loadMaterialTextures(aiMat, aiTextureType_HEIGHT, "texture_normal", scene);
    if (normalMaps.empty()) {
        normalMaps = loadMaterialTextures(aiMat, aiTextureType_NORMALS, "texture_normal", scene);
    }
    if (!normalMaps.empty()) myMaterial->normalMap = normalMaps[0].id;

    // float shininess;
    // if (aiGetMaterialFloat(aiMat, AI_MATKEY_SHININESS, &shininess) == AI_SUCCESS) {
    //     myMaterial->shininess = shininess;
    // }

    return Mesh(vertices, indices, myMaterial);
}

void Model::SetShader(Shader* shader)
{
    for (auto& mesh : meshes) {
        if (mesh.material) {
            mesh.material->shader = shader;
        }
    }
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

        const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());

        if (embeddedTexture)
        {
            texture.id = ResourceManager::LoadTexture(str.C_Str(), "", embeddedTexture);
        }
        else
        {
            texture.id = ResourceManager::LoadTexture(str.C_Str(), this->directory);
        }

        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
    }
    return textures;
}

//
//
//std::unique_ptr<Model> Model::createOrbit(float radius, int segments, float tiltDegrees, float scale, vector<Texture>* textures)
//{
//    auto orbit = std::make_unique<Model>();
//    orbit->transform.setLocalScale(glm::vec3(scale));
//
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//
//    float tilt = glm::radians(tiltDegrees);
//
//    for (int i = 0; i <= segments; ++i)
//    {
//        float angle = (float)i / segments * 2.0f * 3.14159265f;
//
//        float x = radius * cos(angle);
//        float y = 0.0f;
//        float z = radius * sin(angle);
//
//        glm::vec3 p(
//            x,
//            y * cos(tilt) - z * sin(tilt),
//            y * sin(tilt) + z * cos(tilt)
//        );
//
//        Vertex v{};
//        v.Position = p;
//        v.Normal = glm::vec3(0, 1, 0);
//        v.TexCoords = glm::vec2((float)i / segments, 0.0f);
//
//        vertices.push_back(v);
//        indices.push_back(i);
//    }
//    vector<Texture> finalTextures;
//
//    if (textures != nullptr)
//        finalTextures = *textures;
//
//    Mesh orbitMesh(vertices, indices, finalTextures);
//    orbitMesh.meshType = Mesh::MESH_LINES;
//
//    orbit->meshes.push_back(std::move(orbitMesh));
//    return orbit;
//}
//
//std::unique_ptr<Model> Model::createSphere(int rings, int sectors, const std::string& texturePath)
//{
//    auto model = std::make_unique<Model>();
//
//    std::vector<float> ringIDs;
//    for (int r = 0; r < rings; ++r)
//        ringIDs.push_back((float)r);
//    Mesh mesh(ringIDs);
//    mesh.meshType = Mesh::MESH_POINTS;
//    if (!texturePath.empty())
//    {
//        size_t pos = texturePath.find_last_of("/\\");
//        std::string directory = "";
//        std::string filename = texturePath;
//        if (pos != std::string::npos)
//        {
//            directory = texturePath.substr(0, pos);
//            filename = texturePath.substr(pos + 1);
//        }
//
//        Texture tex;
//        tex.id = ResourceManager::LoadTexture(filename, directory);
//        tex.type = "texture_diffuse";
//        tex.path = texturePath;
//        mesh.textures.push_back(tex);
//    }
//    model->meshes.push_back(mesh);
//
//    return model;
//}