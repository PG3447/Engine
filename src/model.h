#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh_data.h"
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

struct MeshNode {
    std::shared_ptr<MeshData> cpuData;
    std::shared_ptr<RenderMesh> gpuMesh;
    std::shared_ptr<Material> material;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Model
{
public:
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    // model data 
    std::vector<MeshNode> nodes;
    Transform transform;
    vector<Model> children;
    string name;
    string directory;
    bool gammaCorrection;
    bool instancingPrepared = false;
    unsigned int instanceVBO = 0;
    
    Model();
    ~Model();
    Model(const std::string& path, bool gamma = false);

    void PrepareInstancing();

    void Draw(GLsizei instanceCount = 0, Material* materialOverride = nullptr);

    static std::unique_ptr<Model> createOrbit(float radius, int segments = 100, float tiltDegrees = 0.0f, float scale = 1.0f, vector<Texture>* textures = nullptr);

    static std::unique_ptr<Model> createSphere(int rings = 10, int sectors = 10, const std::string& texturePath = "");

    void turnOnReflect(unsigned int cubemapTexture);

    MeshNode processMesh(aiMesh* mesh, const aiScene* scene);

    void SetShader(Shader* shader);

private:
    void loadModel(const std::string& path);
    Model processNode(aiNode* node, const aiScene* scene);

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
};


#endif