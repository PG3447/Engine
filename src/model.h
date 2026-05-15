#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "render_mesh.h"
#include "material.h"
#include <shader.h>
#include <transform.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using namespace std;


struct Texture {
    unsigned int id;
    std::string type;
    std::string path;

    bool hasAlpha = false;
};

//
//struct Mesh {
//    std::vector<MeshNode> meshes;
//};

//MeshNode to tak naprawde dane nie zaladowane do RenderComponent
struct MeshNode {
    //bool instancingPrepared = false;
    //unsigned int instanceVBO = 0;

    std::shared_ptr<MeshData> cpuData;
    std::shared_ptr<RenderMesh> gpuMesh;
    std::shared_ptr<Material> material;
    AABB aabb;
    //int indexMaterial;
};


//Model Node to tak naprawde przechowywany GameObject
struct ModelNode
{
    std::string name;
    Transform transform;

    std::vector<MeshNode> meshes;
    //std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<ModelNode>> children;
};

class Model
{
public:
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    // model data 
    std::shared_ptr<ModelNode> rootNode;

    //std::vector<MeshNode> nodes;
    //Transform transform;
    //vector<std::unique_ptr<Model>> children;
    string name;
    string directory;
    bool gammaCorrection;
    //bool instancingPrepared = false;
    //unsigned int instanceVBO = 0;
    
    Model();
    ~Model();
    Model(const std::string& path, bool gamma = false);

    void PrepareInstancing();

    void Draw(GLsizei instanceCount = 0, Material* materialOverride = nullptr);

    void turnOnReflect(unsigned int cubemapTexture);

    MeshNode processMesh(aiMesh* mesh, const aiScene* scene);

    void SetShaderRecursive(ModelNode* node, Shader* shader);

    void SetShader(Shader* shader);


    //AABB GetLocalAABB() const {
    //    AABB result;
    //    for (auto& node : nodes) {
    //        result.min = glm::min(result.min, node.aabb.min);
    //        result.max = glm::max(result.max, node.aabb.max);
    //    }
    //    return result;
    //}

   // int GetTriangleCount() const {

    int CountTriangles(const ModelNode* node) const
    {
        int total = 0;

        for (const auto& mesh : node->meshes)
        {
            if (mesh.cpuData)
                total += mesh.cpuData->indices.size() / 3;
        }

        for (const auto& child : node->children)
        {
            total += CountTriangles(child.get());
        }

        return total;
    }

    int GetTriangleCount() const
    {
        if (!rootNode)
            return 0;

        return CountTriangles(rootNode.get());
    }

    //int GetTriangleCount() const {
    //    int total = 0;
    //    for (auto& node : nodes)
    //        if (node.cpuData)
    //            total += node.cpuData->indices.size() / 3;
    //    return total;
    //}

private:
    void loadModel(const std::string& path);
    std::shared_ptr<ModelNode> processNode(aiNode* node, const aiScene* scene);

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
};


#endif