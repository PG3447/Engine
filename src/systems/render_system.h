#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/ecs.h"


class RenderSystem : public System {
private:
	Query<TransformComponent, RenderComponent>* query;

public:
    RenderSystem(ECS& ecs);


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
};

#endif