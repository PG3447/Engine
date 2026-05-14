#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "model.h"
#include "mesh_data.h"
#include "render_mesh.h"
#include "shader.h"

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>

#include <iostream>
#include <filesystem>

#include "resource_manager.h"

Model::Model() : gammaCorrection(false)
{
}

Model::~Model() {
    //if (instanceVBO != 0) {
    //    glDeleteBuffers(1, &instanceVBO);
    //}
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

//
//void Model::PrepareInstancing()
//{
//    if (instancingPrepared) return;
//
//    if (instanceVBO == 0) {
//        glGenBuffers(1, &instanceVBO);
//    }
//
//    for (auto& node : nodes) {
//        node.gpuMesh->EnableInstancing(instanceVBO);
//    }
//
//    instancingPrepared = true;
//}


// draws the model, and thus all its meshes
//void Model::Draw(GLsizei instanceCount, Material* materialOverride)
//{
//    for (auto& node : nodes)
//    {
//        Material* activeMaterial = materialOverride ? materialOverride : node.material.get();
//        if (activeMaterial) {
//            activeMaterial->Apply();
//        }
//
//        node.gpuMesh->Draw(instanceCount);
//    }
//}

void Model::turnOnReflect(unsigned int cubemapTexture)
{
    //for (unsigned int i = 0; i < meshes.size(); i++)
    //{
    //    meshes[i].reflect = true;
    //    meshes[i].cubemapTexture = cubemapTexture;
    //}
}

void Model::loadModel(string const& path)
{
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_OptimizeMeshes
    ); 
    if (!scene || !scene->mRootNode || ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && !scene->HasAnimations()))
    {
        spdlog::error("ERROR::ASSIMP:: {}", importer.GetErrorString());
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    name = path;

    std::shared_ptr<ModelNode> rootNode = processNode(scene->mRootNode, scene);
    this->rootNode = std::move(rootNode);

    aiMatrix4x4 globalTransform = scene->mRootNode->mTransformation;

    glm::mat4 globalMat(
        globalTransform.a1, globalTransform.b1, globalTransform.c1, globalTransform.d1,
        globalTransform.a2, globalTransform.b2, globalTransform.c2, globalTransform.d2,
        globalTransform.a3, globalTransform.b3, globalTransform.c3, globalTransform.d3,
        globalTransform.a4, globalTransform.b4, globalTransform.c4, globalTransform.d4
    );

    this->skeleton.globalInverseTransform = glm::inverse(globalMat);

    int nodeCounter = 0;
    ReadSkeletonHierarchy(scene->mRootNode, this->skeleton.rootNode, nodeCounter);
    this->skeleton.totalNodes = nodeCounter;

    LoadAnimations(scene);

    //this->nodes = std::move(rootNode->nodes);
    //this->children = std::move(rootNode->children);
    //this->transform = rootNode->transform;
}

std::shared_ptr<ModelNode> Model::processNode(aiNode* node, const aiScene* scene)
{
    auto model = std::make_unique<ModelNode>();

    aiVector3D scale, pos;
    aiQuaternion rot;
    node->mTransformation.Decompose(scale, rot, pos);

    model->name = node->mName.C_Str();
    //model->directory = this->directory;

    model->transform.setLocalPosition({ pos.x, pos.y, pos.z });
    glm::quat q(rot.w, rot.x, rot.y, rot.z);

    glm::vec3 euler = glm::eulerAngles(q); // radiany
    euler = glm::degrees(euler);

    model->transform.setLocalRotation(euler);
    //model.transform.setLocalRotation({ glm::degrees(rot.x), glm::degrees(rot.y), glm::degrees(rot.z) });
    model->transform.setLocalScale({ scale.x, scale.y, scale.z });

    model->meshes.reserve(node->mNumMeshes);
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model->meshes.push_back(processMesh(mesh, scene));
    }

    model->children.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        model->children.push_back(processNode(node->mChildren[i], scene));
    }

    return model;
}

MeshNode Model::processMesh(aiMesh* mesh, const aiScene* scene)
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

        SetVertexBoneDataToDefault(vertex);

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

    ExtractBoneWeightForVertices(vertices, mesh, scene);

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


    std::shared_ptr<MeshData> cpuData = std::make_shared<MeshData>();
    cpuData->vertices = std::move(vertices);
    cpuData->indices = std::move(indices);

    std::shared_ptr<RenderMesh> gpuMesh = std::make_shared<RenderMesh>(*cpuData);

    MeshNode node;
    node.cpuData = cpuData;
    node.gpuMesh = gpuMesh;
    node.material = myMaterial;

    AABB aabb;
    for (auto& v : cpuData->vertices) {
        aabb.min = glm::min(aabb.min, v.Position);
        aabb.max = glm::max(aabb.max, v.Position);
    }
    node.aabb = aabb;

    return node;
}

//void Model::SetShader(Shader* shader)
//{
//    for (auto& node : nodes) {
//        if (node.material) {
//            node.material->shader = shader;
//        }
//    }
//}

void Model::SetShader(Shader* shader)
{
    if (!rootNode)
        return;

    SetShaderRecursive(rootNode.get(), shader);
}

void Model::SetShaderRecursive(ModelNode* node, Shader* shader)
{
    for (auto& mesh : node->meshes)
    {
        if (mesh.material)
        {
            mesh.material->shader = shader;
        }
    }

    for (auto& child : node->children)
    {
        SetShaderRecursive(child.get(), shader);
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

void Model::SetVertexBoneDataToDefault(Vertex& vertex)
{
    for (int i = 0; i < 4; i++)
    {
        vertex.m_BoneIDs[i] = 0;
        vertex.m_Weights[i] = 0.0f;
    }
}

void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
{
    struct VertexWeight {
        int boneID;
        float weight;
    };

    std::vector<std::vector<VertexWeight>> tempVertexWeights(vertices.size());

    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (!skeleton.HasBone(boneName))
        {
            BoneInfo newBoneInfo;
            newBoneInfo.id = skeleton.boneCount;

            aiMatrix4x4 aiMat = mesh->mBones[boneIndex]->mOffsetMatrix;
            newBoneInfo.offset = glm::mat4(
                aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
                aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
                aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
                aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
            );

            skeleton.boneMap[boneName] = newBoneInfo;
            boneID = skeleton.boneCount;
            skeleton.boneCount++;
        }
        else
        {
            boneID = skeleton.boneMap[boneName].id;
        }

        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;

            tempVertexWeights[vertexId].push_back({ boneID, weight });
        }
    }

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        auto& weightsList = tempVertexWeights[i];

        std::sort(weightsList.begin(), weightsList.end(), [](const VertexWeight& a, const VertexWeight& b) {
            return a.weight > b.weight;
        });

        int limit = std::min((int)weightsList.size(), 4);
        float totalWeight = 0.0f;
        for (int j = 0; j < limit; ++j) {
            totalWeight += weightsList[j].weight;
        }

        for (int j = 0; j < limit; ++j) {
            vertices[i].m_BoneIDs[j] = weightsList[j].boneID;

            if (totalWeight > 0.0f) {
                vertices[i].m_Weights[j] = weightsList[j].weight / totalWeight;
            }
            else {
                vertices[i].m_Weights[j] = 0.0f;
            }
        }
    }
}

void Model::ReadSkeletonHierarchy(aiNode* srcNode, SkeletonNode& destNode, int& nodeCounter)
{
    destNode.name = srcNode->mName.data;
    destNode.nodeIndex = nodeCounter++;

    aiMatrix4x4 aiMat = srcNode->mTransformation;
    destNode.localTransform = glm::mat4(
        aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
        aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
        aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
        aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
    );

    aiVector3D pos, scale;
    aiQuaternion rot;
    aiMat.Decompose(scale, rot, pos);

    destNode.defaultPosition = glm::vec3(pos.x, pos.y, pos.z);
    destNode.defaultRotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
    destNode.defaultScale = glm::vec3(scale.x, scale.y, scale.z);

    destNode.children.resize(srcNode->mNumChildren);
    for (unsigned int i = 0; i < srcNode->mNumChildren; i++)
    {
        ReadSkeletonHierarchy(srcNode->mChildren[i], destNode.children[i], nodeCounter);
    }
}

void Model::LoadAnimations(const aiScene* scene)
{
    if (!scene->HasAnimations())
        return;

    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
    {
        aiAnimation* aiAnim = scene->mAnimations[i];
        AnimationClip clip;

        clip.name = aiAnim->mName.data;
        clip.duration = (float)aiAnim->mDuration;
		clip.ticksPerSecond = aiAnim->mTicksPerSecond != 0.0 ? (float)aiAnim->mTicksPerSecond : 24.0f; //24 fps domyslnie, jesli brak informacji w pliku

        for (unsigned int j = 0; j < aiAnim->mNumChannels; j++)
        {
            aiNodeAnim* channel = aiAnim->mChannels[j];
            std::string boneName = channel->mNodeName.data;

            AnimationChannel myChannel;
            myChannel.boneName = boneName;

            ReadKeyframes(channel, myChannel);

            clip.channels[boneName] = myChannel;
        }

        this->animations.push_back(clip);
    }
}

void Model::ReadKeyframes(aiNodeAnim* channel, AnimationChannel& destChannel)
{
    // position keyframes
    for (unsigned int i = 0; i < channel->mNumPositionKeys; i++)
    {
        KeyPosition key;
        key.position = glm::vec3(channel->mPositionKeys[i].mValue.x,
            channel->mPositionKeys[i].mValue.y,
            channel->mPositionKeys[i].mValue.z);
        key.timeStamp = (float)channel->mPositionKeys[i].mTime;
        destChannel.positions.push_back(key);
    }

	// rotation keyframes (quaternions WXYZ)
    for (unsigned int i = 0; i < channel->mNumRotationKeys; i++)
    {
        KeyRotation key;
        key.orientation = glm::quat(channel->mRotationKeys[i].mValue.w,
            channel->mRotationKeys[i].mValue.x,
            channel->mRotationKeys[i].mValue.y,
            channel->mRotationKeys[i].mValue.z);
        key.timeStamp = (float)channel->mRotationKeys[i].mTime;
        destChannel.rotations.push_back(key);
    }

	// scaling keyframes
    for (unsigned int i = 0; i < channel->mNumScalingKeys; i++)
    {
        KeyScale key;
        key.scale = glm::vec3(channel->mScalingKeys[i].mValue.x,
            channel->mScalingKeys[i].mValue.y,
            channel->mScalingKeys[i].mValue.z);
        key.timeStamp = (float)channel->mScalingKeys[i].mTime;
        destChannel.scales.push_back(key);
    }
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