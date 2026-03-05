#ifndef PREFAB_H
#define PREFAB_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <entity.h>
#include <vector>
#include <shader.h>
#include <model.h>
#include <string>

using namespace std;

class Prefab
{
public:

	Model rootModels;
	string directory;
	bool gammaCorrection;
	float meshScale;

    Prefab() : meshScale(1.0f), gammaCorrection(false)
    {

    }


    Prefab(string const& path, float meshScale, bool gamma) : meshScale(meshScale), gammaCorrection(gamma)
    {
        if (!path.empty())
        {
            loadModel(path);
        }
    }

    Entity* getEntitiesCreate(Shader* shader, Light* light = nullptr)
    {
        Entity* rootEntity = createEntityRecursive(&rootModels, nullptr, shader, light);
        return rootEntity;
    }

    Entity* createEntityRecursive(Model* model, Entity* parent, Shader* shader, Light* light)
    {
        if (!model) return nullptr;

        Entity* entity = new Entity();
        entity->transform = model->transform;
        entity->pModel = model;
        entity->pShader = shader;
        entity->pLight = light;
        entity->name = model->name;

        if (parent)
            parent->addChild(entity);


        for (auto& childModel : model->children)
            createEntityRecursive(&childModel, entity, shader, light);

        return entity;
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }

        std::cout << "Scene loaded successfully, root node has " << scene->mRootNode->mNumChildren << " children\n";

        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        rootModels = processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    Model processNode(aiNode* node, const aiScene* scene)
    {
        Model model = Model();
        aiVector3D scale, pos;
        aiQuaternion rot;
        node->mTransformation.Decompose(scale, rot, pos);

        model.name = node->mName.C_Str();
        model.directory = directory;
        model.meshScale = meshScale;

        model.transform.setLocalPosition({ pos.x, pos.y, pos.z });
        model.transform.setLocalRotation({ glm::degrees(rot.x), glm::degrees(rot.y), glm::degrees(rot.z) });
        model.transform.setLocalScale({ scale.x, scale.y, scale.z });

        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            model.meshes.push_back(model.processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            model.children.push_back(processNode(node->mChildren[i], scene));
        }

        return model;
    }

};

#endif