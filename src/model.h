#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <mesh.h>
#include <shader.h>
#include <transform.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, aiTexture* aiTex = nullptr, bool gamma = false);

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh> meshes;
    Transform transform;
    vector<Model> children;
    string name;
    string directory;
    bool gammaCorrection;
    float meshScale;
    unsigned int instanceVBO = 0;
    
    Model();
    //Model(const std::string& path, float meshScale = 1.0f, bool gamma = false);

    void Draw(Shader& shader, GLsizei instanceCount = 0);

    static std::unique_ptr<Model> createOrbit(float radius, int segments = 100, float tiltDegrees = 0.0f, float scale = 1.0f, vector<Texture>* textures = nullptr);

    static std::unique_ptr<Model> createSphere(int rings = 10, int sectors = 10, const std::string& texturePath = "");

    void turnOnReflect(unsigned int cubemapTexture);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

private:

    //void loadModel(const std::string& path);

    //void processNode(aiNode* node, const aiScene* scene);


    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
};


#endif