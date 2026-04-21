#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/ecs.h"

#include <model.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "skybox_renderer.h"
#include "../utils/camera_helper.h"



class RenderSystem : public System {
private:
    struct pair_hash {
        std::size_t operator()(const std::pair<Model*, Shader*>& p) const {
            return std::hash<Model*>()(p.first) ^ (std::hash<Shader*>()(p.second) << 1);
        }
    };

    Query<TransformComponent, RenderComponent>* renderQuery;
    Query<CameraComponent>* cameraQuery;

    std::unordered_map<std::pair<Model*, Shader*>, std::vector<size_t>, pair_hash> instancedGroups;

    bool groupsDirty = true;

    GLFWwindow* window = nullptr;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    GLuint texture;

    SkyboxRenderer skybox;

    glm::mat4 projection;
    glm::mat4 view;

public:
    RenderSystem(ECS& ecs, GLFWwindow* win) : window(win) 
    {
        renderQuery = ecs.CreateQuery<TransformComponent, RenderComponent>();
        cameraQuery = ecs.CreateQuery<CameraComponent>();
        
        Init();
    }

    void Init() {
        glEnable(GL_DEPTH_TEST);
        skybox.Init();
    }

    void OnGameObjectUpdated(GameObject* e) override {
        renderQuery->OnGameObjectUpdated(e); // forward do query
        cameraQuery->OnGameObjectUpdated(e); // forward do query

        groupsDirty = true;
    }

    void MarkDirty() {
        groupsDirty = true;
    }

    void Update(ECS& ecs) override {

        // OpenGL Rendering code goes here
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //bind texture
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, texture);

        BuildGroups();

        RenderAllCameras();

        //BuildGroups();
        //UpdateCamera();
        //RenderGroups();

        //glBindVertexArray(0);

        //skybox.Render(view, projection);
    }


    //void UpdateCamera() {
    //    auto& cameras = std::get<0>(cameraQuery->componentsVectors);

    //    int display_w, display_h;
    //    glfwGetFramebufferSize(window, &display_w, &display_h);

    //    for (size_t i = 0; i < cameras.size(); i++) {
    //        if (cameras[i]->isActive) {
    //            
    //            view = cameras[i]->camera.beginRender(display_w, display_h);

    //            projection = cameras[i]->camera.getProjectionMatrix(
    //                cameras[i]->nearPlane,
    //                cameras[i]->farPlane
    //            );

    //            //float fov = cameras[i]->camera.Zoom;
    //            //float aspect = (float)display_w / (float)display_h;
    //            //
    //            //projection = glm::perspective(glm::radians(fov), aspect, cameras[i]->nearPlane, cameras[i]->farPlane);
    //            //view = cameras[i]->camera.GetViewMatrix();

    //            return;
    //        }
    //    }
    //}

    void ApplyViewport(const Viewport& vp, int w, int h)
    {
        glViewport(
            (GLint)(vp.x * w),
            (GLint)(vp.y * h),
            (GLsizei)(vp.width * w),
            (GLsizei)(vp.height * h)
        );
    }

    void RenderAllCameras() {
        auto& cameras = std::get<0>(cameraQuery->componentsVectors);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        for (size_t i = 0; i < cameras.size(); i++) {
            if (!cameras[i]->isActive)
                continue;

            RenderCamera(*cameras[i], display_w, display_h);
        }
    }

    void RenderCamera(CameraComponent& cam, int width, int height) {
        
        ApplyViewport(cam.viewport, width, height);

        //view = cam.camera.beginRender(width, height);
        view = CameraHelper::getViewMatrix(cam);
        projection = CameraHelper::getProjectionMatrix(cam);
        //projection = cam.camera.getProjectionMatrix(
        //    cam.nearPlane,
        //    cam.farPlane
        //);

        RenderGroups();

        glBindVertexArray(0);
        
        skybox.Render(view, projection);
    }

    void BuildGroups() {
        if (!groupsDirty) return;

        auto& renderers = std::get<1>(renderQuery->componentsVectors);

        instancedGroups.clear();

        for (size_t i = 0; i < renderQuery->gameobjects.size(); i++) {
            RenderComponent* r = renderers[i];

            if (!r || !r->model)
                continue;

            Shader* shader = r->shader;

            std::pair<Model*, Shader*> key = { r->model, shader };
            instancedGroups[key].push_back(i);
        }

        groupsDirty = false;
    }


    void RenderGroups() {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);
        auto& renderers = std::get<1>(renderQuery->componentsVectors);

        for (auto& [key, indices] : instancedGroups) {
            Model* model = key.first;
            Shader* shader = key.second;

            shader->use();
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);

            if (indices.size() == 1) {
                size_t i = indices[0];

                shader->setBool("useInstance", false);
                shader->setMat4("model", transforms[i]->modelMatrix);
                spdlog::info("renderowanie");
                model->Draw(*shader);
            }
            else {
                shader->setBool("useInstance", true);
                RenderInstanced(model, shader, indices);
            }
        }
    }

    void RenderInstanced(Model* model, Shader* shader, std::vector<size_t>& indices)
    {
        auto& transforms = std::get<0>(renderQuery->componentsVectors);

        size_t count = indices.size();
        std::vector<glm::mat4> matrices(count);

        for (size_t i = 0; i < count; i++) {
            matrices[i] = transforms[indices[i]]->modelMatrix;
        }
        
        if (model->instanceVBO == 0)
            glGenBuffers(1, &model->instanceVBO);

        model->PrepareInstancing();
        
        glBindBuffer(GL_ARRAY_BUFFER, model->instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::mat4), matrices.data(), GL_DYNAMIC_DRAW);

        model->Draw(*shader, (GLsizei)count);
    }


    //void RenderSkybox()
    //{
    //    //glDepthFunc(GL_LEQUAL);

    //    //glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    //    //
    //    //skyboxShader->use();
    //    //skyboxShader->setMat4("view", skyboxView);
    //    //skyboxShader->setMat4("projection", projection);
    //    //
    //    //glBindVertexArray(skyboxVAO);
    //    //glActiveTexture(GL_TEXTURE0);
    //    //glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    //    //
    //    //glDrawArrays(GL_TRIANGLES, 0, 36);
    //    //
    //    //glBindVertexArray(0);
    //    //glDepthFunc(GL_LESS);
    //}

};

#endif


/*
for (Camera* cam : { &camera, &cameraRight }) {
    glm::mat4 view = cam->beginRender(display_w, display_h);
    glm::mat4 projection = cam->getProjectionMatrix(0.1f, 10000.0f);

    //activating program object
    ourShader->use();
    renderGroup(projection, view, systemModel, cam->Position);


    // draw skybox as last
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    skyboxShader->use();
    view = glm::mat4(glm::mat3(cam->GetViewMatrix())); // remove translation from the view matrix
    skyboxShader->setMat4("view", view);
    skyboxShader->setMat4("projection", projection);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
    //OLD RENDER FUNCTION ENDS HERE
}
*/

/*
* 


struct pair_hash {
    std::size_t operator()(const std::pair<Model*, Shader*>& p) const {
        return std::hash<Model*>()(p.first) ^ (std::hash<Shader*>()(p.second) << 1);
    }
};


std::unordered_map<std::pair<Model*, Shader*>, std::vector<Entity*>, pair_hash> instancedGroups;

void startGroupInstanced(Entity* root)
{
    instancedGroups.clear();
    std::vector<Entity*> stackEntity;
    stackEntity.push_back(root);

    while (!stackEntity.empty())
    {
        Entity* entity = stackEntity.back();
        stackEntity.pop_back();

        if (entity->pModel)
        {
            std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
            instancedGroups[key].push_back(entity);
        }

        for (auto& child : entity->children)
        {
            stackEntity.push_back(child.get());
        }
    }
    ;
}

void AddEntityToGroupInstanced(Entity* entity)
{
    if (!entity->pModel)
        return;

    std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
    std::vector<Entity*>& group = instancedGroups[key];

    group.push_back(entity);

}

bool start = true;
bool change = false;

void renderGroup(glm::mat4 projection, glm::mat4 view, glm::mat4 systemModel, const glm::vec3& viewPos)
{
    for (auto& [key, entities] : instancedGroups)
    {
        Model* model = key.first;
        Shader* shader = key.second;

        shader->use();

        shader->setVec3("viewPos", viewPos);
        shader->setVec3("cameraPos", viewPos);

        shader->setMat4("projection", projection);
        shader->setMat4("view", view);

        if (entities.size() == 0)
        {
            continue;
        }
        else if (entities.size() == 1)
        {
            shader->setBool("useInstance", false);
            shader->setMat4("model", systemModel * entities[0]->transform.getModelMatrix());

            if (entities[0]->pLight != nullptr)
            {
                entities[0]->transform.setLocalPosition(entities[0]->pLight->position);
                glm::vec3 dir = glm::normalize(entities[0]->pLight->direction);

                float yaw = atan2(dir.x, dir.z);
                float pitch = asin(-dir.y);

                entities[0]->transform.setLocalRotation(glm::degrees(glm::vec3(pitch, yaw, 0.0f)));
                entities[0]->pLight->Apply(*shader);
            }

            if (model != nullptr)
            {
                model->Draw(*shader);
            }
        }
        else
        {
            shader->setBool("useInstance", true);
            renderInstanced(model, shader, entities);

        }
    }
    start = false;
    change = false;
}


void renderInstanced(Model* model, Shader* shader, std::vector<Entity*>& entities)
{
    if (start || change) {
        size_t numEntities = entities.size();
        glm::mat4* modelMatrices = new glm::mat4[numEntities];

        for (size_t i = 0; i < numEntities; ++i)
        {
            modelMatrices[i] = entities[i]->transform.getModelMatrix();
        }


        if (model->instanceVBO == 0)
            glGenBuffers(1, &model->instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, model->instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, numEntities * sizeof(glm::mat4), modelMatrices, GL_DYNAMIC_DRAW);
    }

    if (start) {
        for (unsigned int i = 0; i < model->meshes.size(); i++)
        {
            unsigned int VAO = model->meshes[i].VAO;
            glBindVertexArray(VAO);

            // matrix
            GLsizei vec4Size = sizeof(glm::vec4);
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
            glEnableVertexAttribArray(9);
            glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
            glEnableVertexAttribArray(10);
            glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

            glVertexAttribDivisor(7, 1);
            glVertexAttribDivisor(8, 1);
            glVertexAttribDivisor(9, 1);
            glVertexAttribDivisor(10, 1);

            glBindVertexArray(0);
        }
    }
    if (model != nullptr)
    {
        model->Draw(*shader, (GLsizei)entities.size());
    }
}

*/

/*

    void render()
    {
        //OLD RENDER FUNCION STARTS HERE

            //float time = glfwGetTime();
            //float radius = 10.0f;
            //float speed = 2.0f;


            //pointLight->position.x = radius * cos(speed * time);
            //pointLight->position.y = 1.0f;
            //pointLight->position.z = radius * sin(speed * time);

        if (!mouseMove)
        {
            updateFollowCamera();
        }

        // OpenGL Rendering code goes here
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        //activating program object
        ourShader->use();


        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)display_w / (float)display_h, 0.1f, 10000.0f);
        glm::mat4 view = glm::mat4(1.0f);
        view = camera.GetViewMatrix();


        // render the loaded modeld
        root->updateSelfAndChild();
        glm::mat4 systemRotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotationX), glm::vec3(1, 0, 0));
        glm::mat4 systemRotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotationY), glm::vec3(0, 1, 0));
        glm::mat4 systemModel = systemRotationY * systemRotationX;
        renderGroup(projection, view, systemModel);
        //ourEntity->drawSelfAndChild(*ourShader, projection, view, sphereRadius, sphereRings, sphereSectors, systemModel);


        //glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        //    skyboxShader->use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        //    skyboxShader->setMat4("view", view);
        //    skyboxShader->setMat4("projection", projection);
            // skybox cube
        //    glBindVertexArray(skyboxVAO);
        //    glActiveTexture(GL_TEXTURE0);
        //    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        //    glDrawArrays(GL_TRIANGLES, 0, 36);
        //    glBindVertexArray(0);
        ///    glDepthFunc(GL_LESS); // set depth function back to default*/
        //OLD RENDER FUNCTION ENDS HERE
/*
triangleShader->use();
glBindVertexArray(triangleVAO);
glDrawArrays(GL_TRIANGLES, 0, 3);
glBindVertexArray(0);

    }




 //std::unordered_map<std::pair<Model*, Shader*>, std::vector<Entity*>, pair_hash> instancedGroups;

    void startGroupInstanced(Entity* root)
    {
        instancedGroups.clear();
        std::vector<Entity*> stackEntity;
        stackEntity.push_back(root);

        while (!stackEntity.empty())
        {
            Entity* entity = stackEntity.back();
            stackEntity.pop_back();

            if (entity->pModel)
            {
                std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
                instancedGroups[key].push_back(entity);
            }

            for (auto& child : entity->children)
            {
                stackEntity.push_back(child.get());
            }
        }
        ;
    }

    void AddEntityToGroupInstanced(Entity* entity)
    {
        if (!entity->pModel)
            return;

        std::pair<Model*, Shader*> key = { entity->pModel, entity->pShader };
        std::vector<Entity*>& group = instancedGroups[key];

        group.push_back(entity);

    }

    bool start = true;
    bool change = false;

    void renderGroup(glm::mat4 projection, glm::mat4 view, glm::mat4 systemModel)
    {
        for (auto& [key, entities] : instancedGroups)
        {
            Model* model = key.first;
            Shader* shader = key.second;

            shader->use();

            shader->setVec3("viewPos", camera.Position);
            shader->setVec3("cameraPos", camera.Position);

            shader->setMat4("projection", projection);
            shader->setMat4("view", view);

            if (entities.size() == 0)
            {
                continue;
            }
            else if (entities.size() == 1)
            {
                shader->setBool("useInstance", false);
                shader->setMat4("model", systemModel * entities[0]->transform.getModelMatrix());

                if (entities[0]->pLight != nullptr)
                {
                    entities[0]->transform.setLocalPosition(entities[0]->pLight->position);
                    glm::vec3 dir = glm::normalize(entities[0]->pLight->direction);

                    float yaw = atan2(dir.x, dir.z);
                    float pitch = asin(-dir.y);

                    entities[0]->transform.setLocalRotation(glm::degrees(glm::vec3(pitch, yaw, 0.0f)));
                    entities[0]->pLight->Apply(*shader);
                }

                if (model != nullptr)
                {
                    model->Draw(*shader);
                }
            }
            else
            {
                shader->setBool("useInstance", true);
                renderInstanced(model, shader, entities);

            }
        }
        start = false;
        change = false;
    }


    void renderInstanced(Model* model, Shader* shader, std::vector<Entity*>& entities)
    {
        if (start || change) {
            size_t numEntities = entities.size();
            glm::mat4* modelMatrices = new glm::mat4[numEntities];

            for (size_t i = 0; i < numEntities; ++i)
            {
                modelMatrices[i] = entities[i]->transform.getModelMatrix();
            }


            if (model->instanceVBO == 0)
                glGenBuffers(1, &model->instanceVBO);
            glBindBuffer(GL_ARRAY_BUFFER, model->instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, numEntities * sizeof(glm::mat4), modelMatrices, GL_DYNAMIC_DRAW);
        }

        if (start) {
            for (unsigned int i = 0; i < model->meshes.size(); i++)
            {
                unsigned int VAO = model->meshes[i].VAO;
                glBindVertexArray(VAO);

                // matrix
                GLsizei vec4Size = sizeof(glm::vec4);
                glEnableVertexAttribArray(7);
                glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
                glEnableVertexAttribArray(8);
                glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
                glEnableVertexAttribArray(9);
                glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
                glEnableVertexAttribArray(10);
                glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

                glVertexAttribDivisor(7, 1);
                glVertexAttribDivisor(8, 1);
                glVertexAttribDivisor(9, 1);
                glVertexAttribDivisor(10, 1);

                glBindVertexArray(0);
            }
        }
        if (model != nullptr)
        {
            model->Draw(*shader, (GLsizei)entities.size());
        }
    }


*/

/*

    void Update(ECS& ecs) override {
        auto& transforms = std::get<0>(query->componentsVectors);
        auto& renderers = std::get<1>(query->componentsVectors);
        auto& camera = std::get<2>(query->componentsVectors);

        for (size_t i = 0; i < query->gameobjects.size(); i++) {
            CameraComponent* c = camera[i];
            if (c->isActive) {
                TransformComponent* t = transforms[i];
                RenderComponent* r = renderers[i];

                Shader* shaderToUse =  r->shader;// (r->shader) ? r->shader : &r->defaultShader;
                shaderToUse->use();
                shaderToUse->setMat4("projection", c->projection);
                shaderToUse->setMat4("view", c->view);
                shaderToUse->setMat4("model", t->modelMatrix);

                if (r->model)
                    r->model->Draw(*shaderToUse);
            }
        }
    }
*/

/*

    void drawSelfAndChild(Shader& ourShader, glm::mat4 projection, glm::mat4 view, float radius, int rings, int sectors, glm::mat4 systemModel)
    {
        Shader* shaderToUse = (pShader) ? pShader : &ourShader;
        shaderToUse->use();

        shaderToUse->setMat4("projection", projection);
        shaderToUse->setMat4("view", view);
        shaderToUse->setMat4("model", systemModel * transform.getModelMatrix());

        if (pShader) {
            // ustawiamy parametry sfery
            shaderToUse->setFloat("radius", radius);
            shaderToUse->setInt("rings", rings);
            shaderToUse->setInt("sectors", sectors);
        }
        if (pModel)
        {
            pModel->Draw(*shaderToUse);
        }


        for (auto&& child : children)
        {
            child->drawSelfAndChild(ourShader, projection, view, radius, rings, sectors, systemModel);
        }
    }
*/