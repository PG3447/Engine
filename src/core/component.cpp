#include "core/component.h"
#include "core/gameobject.h"
#include "resource_manager.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

uint32_t staticCounterAnimator = 1;
static uint32_t nextAnimatorID = 0;

void AnimatorComponent::OnEnable(GameObject* owner) {
    //owner->GetComponent<RenderComponent>()->animator = this;
    animatorID = nextAnimatorID++;
    staticCounterAnimator++;
    owner->TraverseChildren([this](GameObject* go) {
        auto* render = go->GetComponent<RenderComponent>();
        if (render) {
            render->animator = this;
            render->rendererDirty = true;
            spdlog::info("animator przypisany do: {}", go->name);
        }
        });
}


void ColliderComponent::OnEnable(GameObject* owner) {
    auto renderComponent = owner->GetComponent<RenderComponent>();
    auto transformComponent = owner->GetComponent<TransformComponent>();

    if (!renderComponent || !transformComponent)
        return;

    glm::vec3 localHalfSize = (renderComponent->localObjectAABB.max - renderComponent->localObjectAABB.min) * 0.5f;

    localHalfSize *= transformComponent->scale;

    glm::mat4 rot = glm::yawPitchRoll(glm::radians(transformComponent->rotation.y), glm::radians(transformComponent->rotation.x), glm::radians(transformComponent->rotation.z));

    glm::mat3 absRot = glm::mat3(rot);

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            absRot[i][j] = std::abs(absRot[i][j]);

    halfSize = absRot * localHalfSize;

    glm::mat3 rota = glm::mat3(glm::yawPitchRoll(glm::radians(transformComponent->rotation.y), glm::radians(transformComponent->rotation.x), glm::radians(transformComponent->rotation.z)));
    offset = rota * (renderComponent->localObjectAABB.centerLocal * transformComponent->scale);
    //if (renderComponent != nullptr && transformComponent != nullptr)
    //{
    //    this->halfSize = (renderComponent->localObjectAABB.max - renderComponent->localObjectAABB.min) * 0.5f;
    //    this->halfSize *= transformComponent->scale;
    //    //this->halfSize = renderComponent->localObjectAABB.max * transformComponent->scale;
    //}
}

void ColliderComponent::Recalculate(GameObject* owner)
{
    auto renderComponent = owner->GetComponent<RenderComponent>();
    auto transformComponent = owner->GetComponent<TransformComponent>();

    if (renderComponent == nullptr || transformComponent == nullptr)
        return;

    glm::vec3 localHalfSize = (renderComponent->localObjectAABB.max - renderComponent->localObjectAABB.min) * 0.5f;

    localHalfSize *= transformComponent->scale;

    glm::mat4 rot = glm::yawPitchRoll(glm::radians(transformComponent->rotation.y), glm::radians(transformComponent->rotation.x), glm::radians(transformComponent->rotation.z));

    glm::mat3 absRot(rot);

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            absRot[i][j] = std::abs(absRot[i][j]);
        }
    }

    halfSize = absRot * localHalfSize;

    glm::mat3 rota = glm::mat3(glm::yawPitchRoll(glm::radians(transformComponent->rotation.y), glm::radians(transformComponent->rotation.x), glm::radians(transformComponent->rotation.z)));
    offset = rota * (renderComponent->localObjectAABB.centerLocal * transformComponent->scale);
}

void  RenderComponent::Serialize(YAML::Node& node)
{
    node["type"] = "Render";
    if (!meshes.empty())
        node["pathModel"] = ResourceManager::GetModelPathByMeshNodeID(meshes[0].meshNodeID);

    for (auto& mesh : meshes)
    {
        std::string name = ResourceManager::GetModelPathByMeshNodeID(mesh.meshNodeID);
        node[name]["surfaceType"] = static_cast<int>(mesh.material->surfaceType);
        node[name]["diffuseColor"] = mesh.material->diffuseColor;
        node[name]["diffuseMap"] = ResourceManager::GetTexturePath(mesh.material->diffuseMap);
        node[name]["specularMap"] = ResourceManager::GetTexturePath(mesh.material->specularMap);
        node[name]["metallicRoughness"] = ResourceManager::GetTexturePath(mesh.material->metallicRoughnessMap);
        node[name]["normal"] = ResourceManager::GetTexturePath(mesh.material->normalMap);
        node[name]["ao"] = ResourceManager::GetTexturePath(mesh.material->aoMap);
        node[name]["aoInMetallicRoughness"] = mesh.material->aoInMetallicRoughness;
    }

}


void RenderComponent::Deserialize(const YAML::Node& node)
{
    //meshes.clear();

    //if (!node["type"])
    //    return;

    //if (!node["pathModel"])
    //    return;

    //for (const auto& it : node)
    //{
    //    const std::string& name = it.first.as<std::string>();

    //    // pomijamy pola globalne
    //    if (name == "type" || name == "pathModel")
    //        continue;

    //    //const YAML::Node& meshNode = it.second;

    //    //MeshData mesh;
    //    //std::string modelPath = node["pathModel"].as<std::string>();

    //    //mesh.meshNodeID = ResourceManager::GetMeshNodeIDByModelPath(modelPath);

    //    //// surfaceType
    //    //mesh.material->surfaceType =
    //    //    static_cast<SurfaceType>(meshNode["surfaceType"].as<int>());

    //    //// diffuseColor (vec4 -> dziêki YAML convert)
    //    //if (meshNode["diffuseColor"])
    //    //    mesh.material->diffuseColor =
    //    //    meshNode["diffuseColor"].as<glm::vec4>();

    //    //// textures
    //    //if (meshNode["diffuseMap"])
    //    //    mesh.material->diffuseMap =
    //    //    ResourceManager::LoadTexture(meshNode["diffuseMap"].as<std::string>());

    //    //if (meshNode["ao"])
    //    //    mesh.material->aoMap =
    //    //    ResourceManager::LoadTexture(meshNode["ao"].as<std::string>());

    //    //if (meshNode["metallicRoughness"])
    //    //    mesh.material->metallicRoughnessMap =
    //    //    ResourceManager::LoadTexture(meshNode["metallicRoughness"].as<std::string>());

    //    //if (meshNode["normal"])
    //    //    mesh.material->normalMap =
    //    //    ResourceManager::LoadTexture(meshNode["normal"].as<std::string>());

    //    //// UWAGA: masz duplikat "ao" w serialize — to nadpisuje poprzedni
    //    //// mesh.material->specularMap i aoMap s¹ w konflikcie

    //    //mesh.material->aoInMetallicRoughness =
    //    //    meshNode["aoInMetallicRoughness"].as<bool>();

    //    //meshes.push_back(mesh);
    //}
}