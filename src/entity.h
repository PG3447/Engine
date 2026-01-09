

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

class Transform
{
protected:
	//Local space information
	glm::vec3 m_pos = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_eulerRot = { 0.0f, 0.0f, 0.0f }; //In degrees
	glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };

	//Global space information concatenate in matrix
	glm::mat4 m_modelMatrix = glm::mat4(1.0f);

	//Dirty flag
	bool m_isDirty = true;

protected:
	glm::mat4 getLocalModelMatrix()
	{
		const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
		const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f), glm::radians(m_eulerRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

		// Y * X * Z
		const glm::mat4 rotationMatrix = transformY * transformX * transformZ;

		// translation * rotation * scale (also know as TRS matrix)
		return glm::translate(glm::mat4(1.0f), m_pos) * rotationMatrix * glm::scale(glm::mat4(1.0f), m_scale);
	}
public:

	void computeModelMatrix()
	{
		m_modelMatrix = getLocalModelMatrix();
		m_isDirty = false;
	}

	void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix)
	{
		m_modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
		m_isDirty = false;
	}

	void setLocalPosition(const glm::vec3& newPosition)
	{
		m_pos = newPosition;
		m_isDirty = true;
	}

	void setLocalRotation(const glm::vec3& newRotation)
	{
		m_eulerRot = newRotation;
		m_isDirty = true;
	}

	void setLocalScale(const glm::vec3& newScale)
	{
		m_scale = newScale;
		m_isDirty = true;
	}

	const glm::vec3& getGlobalPosition() const
	{
		return m_modelMatrix[3];
	}

	const glm::vec3& getLocalPosition() const
	{
		return m_pos;
	}

	const glm::vec3& getLocalRotation() const
	{
		return m_eulerRot;
	}

	const glm::vec3& getLocalScale() const
	{
		return m_scale;
	}

	const glm::mat4& getModelMatrix() const
	{
		return m_modelMatrix;
	}

	glm::vec3 getRight() const
	{
		return m_modelMatrix[0];
	}


	glm::vec3 getUp() const
	{
		return m_modelMatrix[1];
	}

	glm::vec3 getBackward() const
	{
		return m_modelMatrix[2];
	}

	glm::vec3 getForward() const
	{
		return -m_modelMatrix[2];
	}

	glm::vec3 getGlobalScale() const
	{
		return { glm::length(getRight()), glm::length(getUp()), glm::length(getBackward()) };
	}

	bool isDirty() const
	{
		return m_isDirty;
	}
};

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

	void setAppearance(const glm::vec3& newColor, float rotSpeed, const glm::vec3& rotAxis)
	{
		//color = newColor;
		//planetRotationSpeed = rotSpeed;
		//planetRotationAxis = rotAxis;
	}


	void initialOrbit(float orbitAngleVal, float orbitSpeedVal, float orbitRadiusVal, float orbitTiltVal)
	{
		//orbitAngle = orbitAngleVal;
		//orbitSpeed = orbitSpeedVal;
		//orbitRadius = orbitRadiusVal;
		//orbitTilt = orbitTiltVal;
	}
	

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

	Entity* getLastChild()
	{
		if (children.empty())
			return nullptr;
		return children.back().get();
	}

	void updateOrbit(float deltaTime)
	{
		//for (auto& child : children)
		//{
		//	if (child->orbitEnabled)
		//		child->orbitAngle += child->orbitSpeed * deltaTime;
		//
		//	// Konwersja k¹tów na radiany
		//	float orbitAngleRad = glm::radians(child->orbitAngle);
		//	float tiltRad = glm::radians(child->orbitTilt);
		//
		//	// Pozycja wzglêdem rodzica
		//	glm::vec3 orbitPos;
		//	orbitPos.x = child->orbitRadius * cos(orbitAngleRad);
		//	orbitPos.y = child->orbitRadius * sin(orbitAngleRad) * sin(tiltRad);
		//	orbitPos.z = child->orbitRadius * sin(orbitAngleRad) * cos(tiltRad);
		//
		//	child->transform.setLocalPosition(orbitPos);
		//
		//
		//	child->planetRotationAngle += child->planetRotationSpeed * deltaTime;
		//
		//	glm::vec3 euler = child->transform.getLocalRotation();
		//	// spin wokó³ osi Y
		//	euler.y = child->planetRotationAngle;
		//
		//	// tilt zostaje w euler.z (ustawiony wczeœniej)
		//	child->transform.setLocalRotation(euler);
		//
		//	// Obrót wokó³ w³asnej osi w formie eulerów (stopnie)
		//	//child->planetRotationAngle += child->planetRotationSpeed * deltaTime;
		//	//glm::vec3 eulerRotation = child->planetRotationAxis * child->planetRotationAngle + child->transform.getLocalRotation().z;
		//	//child->transform.setLocalRotation(eulerRotation);
		//
		//	// Rekurencja
		//	child->updateOrbit(deltaTime);
		//}
	}





	//Update transform if it was changed
	void updateSelfAndChild()
	{
		if (transform.isDirty()) {
			forceUpdateSelfAndChild();
			return;
		}

		for (auto&& child : children)
		{
			child->updateSelfAndChild();
		}
	}

	//Force update of transform even if local space don't change
	void forceUpdateSelfAndChild()
	{
		if (parent)
			transform.computeModelMatrix(parent->transform.getModelMatrix());
		else
			transform.computeModelMatrix();

		for (auto&& child : children)
		{
			child->forceUpdateSelfAndChild();
		}
	}


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

