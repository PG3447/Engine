#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	/*
protected:
	glm::mat4 getLocalModelMatrix(const TransformComponent& comp)
	{
		const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f), glm::radians(comp.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f), glm::radians(comp.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f), glm::radians(comp.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		// Y * X * Z
		const glm::mat4 rotationMatrix = transformY * transformX * transformZ;

		// translation * rotation * scale (TRS matrix)
		return glm::translate(glm::mat4(1.0f), comp.position) *
			rotationMatrix *
			glm::scale(glm::mat4(1.0f), comp.scale);
	}

public:

	void computeModelMatrix(TransformComponent& comp)
	{
		comp.modelMatrix = getLocalModelMatrix(comp);
		comp.isDirty = false;
	}

	// Z macierz¹ rodzica
	void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix, TransformComponent& comp)
	{
		comp.modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix(comp);
		comp.isDirty = false;
	}

	// Ustawianie pozycji
	void setLocalPosition(TransformComponent& comp, const glm::vec3& newPosition)
	{
		comp.position = newPosition;
		comp.isDirty = true;
	}

	// Ustawianie rotacji
	void setLocalRotation(TransformComponent& comp, const glm::vec3& newRotation)
	{
		comp.rotation = newRotation;
		comp.isDirty = true;
	}

	// Ustawianie skali
	void setLocalScale(TransformComponent& comp, const glm::vec3& newScale)
	{
		comp.scale = newScale;
		comp.isDirty = true;
	}

	// Pobranie pozycji globalnej (z macierzy)
	glm::vec3 getGlobalPosition(const TransformComponent& comp) const
	{
		return glm::vec3(comp.modelMatrix[3]);
	}

	const glm::vec3& getLocalPosition(const TransformComponent& comp) const
	{
		return comp.position;
	}

	const glm::vec3& getLocalRotation(const TransformComponent& comp) const
	{
		return comp.rotation;
	}

	const glm::vec3& getLocalScale(const TransformComponent& comp) const
	{
		return comp.scale;
	}

	const glm::mat4& getModelMatrix(const TransformComponent& comp) const
	{
		return comp.modelMatrix;
	}

	glm::vec3 getRight(const TransformComponent& comp) const
	{
		return glm::vec3(comp.modelMatrix[0]);
	}

	glm::vec3 getUp(const TransformComponent& comp) const
	{
		return glm::vec3(comp.modelMatrix[1]);
	}

	glm::vec3 getBackward(const TransformComponent& comp) const
	{
		return glm::vec3(comp.modelMatrix[2]);
	}

	glm::vec3 getForward(const TransformComponent& comp) const
	{
		return -glm::vec3(comp.modelMatrix[2]);
	}

	glm::vec3 getGlobalScale(const TransformComponent& comp) const
	{
		return { glm::length(getRight(comp)), glm::length(getUp(comp)), glm::length(getBackward(comp)) };
	}

	bool isDirty(const TransformComponent& comp) const
	{
		return comp.isDirty;
	} */
	
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
#endif