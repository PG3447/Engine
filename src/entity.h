

#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <list> //std::list
#include <array> //std::array
#include <memory> //std::unique_ptr

#include <model.h>
#include <shader.h>
#include <light.h>
#include <transform.h>


class Entity
{
public:
	//Scene graph
	std::list<std::unique_ptr<Entity>> children;
	Entity* parent = nullptr;

	std::string name = "object";
	Transform transform;
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };

	Model* pModel = nullptr;
	Shader* pShader = nullptr;
	Light* pLight = nullptr;

	// constructor
	Entity(Model* model = nullptr, Shader* shader = nullptr, Light* light = nullptr)
		: pModel{ model }, pShader{ shader }, pLight { light }
	{

	}

	//void setAppearance(const glm::vec3& newColor, float rotSpeed, const glm::vec3& rotAxis)
	//{
	//	//color = newColor;
	//	//planetRotationSpeed = rotSpeed;
	//	//planetRotationAxis = rotAxis;
	//}
	//

	//
	////Add child. Argument input is argument of any constructor that you create. By default you can use the default constructor and don't put argument input.
//template<typename... TArgs>
//void addChild(TArgs&&... args)
//{
//	children.emplace_back(std::make_unique<Entity>(std::forward<TArgs>(args)...));
//	children.back()->parent = this;
//}

	template<typename... TArgs>
	Entity* addChild(TArgs&&... args)
	{
		children.emplace_back(std::make_unique<Entity>(std::forward<TArgs>(args)...));
		Entity* newChild = children.back().get();
		newChild->parent = this;
		return newChild;
	}

	Entity* addChild(Entity* child)
	{
		if (!child) return nullptr;
		child->parent = this;
		children.emplace_back(std::unique_ptr<Entity>(child));
		return child;
	}

	Entity* getLastChild()
	{
		if (children.empty())
			return nullptr;
		return children.back().get();
	}
	
	Entity* findChild(const std::string& searchName)
	{
		for (auto& child : children)
		{
			if (child->name == searchName)
				return child.get();
		}

		for (auto& child : children)
		{
			Entity* found = child->findChild(searchName);
			if (found)
				return found;
		}

		return nullptr;
	}

	//Update transform if it was changed
	//void updateSelfAndChild()
	//{
	//	if (transform.isDirty()) {
	//		forceUpdateSelfAndChild();
	//		return;
	//	}

	//	for (auto&& child : children)
	//	{
	//		child->updateSelfAndChild();
	//	}
	//}

	////Force update of transform even if local space don't change
	//void forceUpdateSelfAndChild()
	//{
	//	if (parent)
	//		transform.computeModelMatrix(parent->transform.getModelMatrix());
	//	else
	//		transform.computeModelMatrix();

	//	for (auto&& child : children)
	//	{
	//		child->forceUpdateSelfAndChild();
	//	}
	//}


	//void drawSelfAndChild(Shader& ourShader, glm::mat4 projection, glm::mat4 view, float radius, int rings, int sectors, glm::mat4 systemModel)
	//{
	//	Shader* shaderToUse = (pShader) ? pShader : &ourShader;
	//	shaderToUse->use();

	//	shaderToUse->setMat4("projection", projection);
	//	shaderToUse->setMat4("view", view);
	//	shaderToUse->setMat4("model", systemModel * transform.getModelMatrix());
	//	
	//	if (pShader) {
	//		// ustawiamy parametry sfery
	//		shaderToUse->setFloat("radius", radius);
	//		shaderToUse->setInt("rings", rings);
	//		shaderToUse->setInt("sectors", sectors);
	//	}
	//	if (pModel)
	//	{
	//		pModel->Draw(*shaderToUse);
	//	}
	//	

	//	for (auto&& child : children)
	//	{
	//		child->drawSelfAndChild(ourShader, projection, view, radius, rings, sectors, systemModel);
	//	}
	//}
};
#endif

