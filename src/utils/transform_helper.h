#ifndef TRANSFORM_HELPER_H
#define TRANSFORM_HELPER_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>


class TransformHelper {
protected:

	//static glm::mat4 getLocalModelMatrix(const TransformComponent& comp)
	//{
	//	const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f), glm::radians(comp.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	//	const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f), glm::radians(comp.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	//	const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f), glm::radians(comp.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	//
	//	// Y * X * Z
	//	const glm::mat4 rotationMatrix = transformY * transformX * transformZ;
	//
	//	// translation * rotation * scale (TRS matrix)
	//	return glm::translate(glm::mat4(1.0f), comp.position) * rotationMatrix * glm::scale(glm::mat4(1.0f), comp.scale);
	//}

	static glm::mat4 getLocalModelMatrix(const TransformComponent& comp)
	{
		glm::mat4 m(1.0f);

		m = glm::translate(m, comp.position);
		m *= glm::toMat4(glm::quat(glm::radians(comp.rotation)));
		m = glm::scale(m, comp.scale);

		return m;
	}

public:

	static void computeModelMatrix(TransformComponent& comp)
	{
		comp.modelMatrix = getLocalModelMatrix(comp);
		comp.isDirty = false;
	}

	// Z macierzą rodzica
	static void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix, TransformComponent& comp)
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
	}
};


#endif