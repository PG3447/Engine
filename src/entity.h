

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
};
#endif

