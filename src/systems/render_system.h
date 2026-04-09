#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/ecs.h"


class RenderSystem : public System {
private:
	Query<TransformComponent, RenderComponent>* query;

public:
    RenderSystem(ECS& ecs);

    void Update(ECS& ecs) override {
        auto& transforms = std::get<0>(query->componentsVectors);
        auto& renderers = std::get<1>(query->componentsVectors);

        for (size_t i = 0; i < query->gameobjects.size(); i++) {
            TransformComponent* t = transforms[i];
            RenderComponent* r = renderers[i];

            // Przelicz modelMatrix jeœli jest dirty
            if (t->isDirty) {
                Transform::computeModelMatrix(*t); // helper statyczny lub instancja
            }

            Shader* shaderToUse = (r->shader) ? r->shader : &r->defaultShader;
            shaderToUse->use();
            shaderToUse->setMat4("projection", r->projection);
            shaderToUse->setMat4("view", r->view);
            shaderToUse->setMat4("model", t->modelMatrix);

            if (r->model)
                r->model->Draw(*shaderToUse);
        }
    }



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

        triangleShader->use();
        glBindVertexArray(triangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

    }


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


};

#endif


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