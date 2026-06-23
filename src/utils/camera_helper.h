#ifndef CAMERA_HELPER_H
#define CAMERA_HELPER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "transform_helper.h"

constexpr float YAW = -90.0f;
constexpr float PITCH = 0.0f;
constexpr  float MovementSpeed = 22.5f;
constexpr  float MouseSensitivity = 0.1f;

class CameraHelper {
public:
    static void InitialCamera(CameraComponent& cam, TransformComponent& transform, glm::vec3 up, Viewport vp) //, float yaw, float pitch
    {
        cam.state.WorldUp = up;
        cam.viewport = vp;

        updateCameraVectors(cam, transform);
    }


    static glm::mat4 getViewMatrix(CameraComponent& cam, TransformComponent& transform) {
        glm::vec3 position = TransformHelper::getGlobalPosition(transform);

        return glm::lookAt(position, position + cam.state.Front, cam.state.WorldUp);
    }

    static glm::mat4 getProjectionMatrix(CameraComponent& cam, int screenWidth, int screenHeight) {
        float vpWidth = cam.viewport.width * screenWidth;
        float vpHeight = cam.viewport.height * screenHeight;

        float aspectRatio = vpWidth / vpHeight;
        return glm::perspective(glm::radians(cam.fov), aspectRatio, cam.nearPlane, cam.farPlane);
    }

    static void updateCameraVectors(CameraComponent& cam, TransformComponent& transform)
    {
        cam.dirty = true;

        cam.state.Front = TransformHelper::getForward(transform);
        cam.state.Right = TransformHelper::getRight(transform);
        cam.state.Up = TransformHelper::getUp(transform);
    }


    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    static void ProcessMouseMovement(CameraComponent& cam, TransformComponent& transform, float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        glm::vec3 rot = transform.rotation;

        rot.y -= xoffset; // yaw
        rot.x -= yoffset; // pitch

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (rot.x > 89.0f)
                rot.x = 89.0f;
            if (rot.x < -89.0f)
                rot.x = -89.0f;
        }

        transform.rotation = rot;
        transform.isDirty = true;

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors(cam, transform);
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(CameraComponent& cam, float yoffset)
    {
        cam.dirty = true;
        cam.fov -= yoffset;
        if (cam.fov < 1.0f)
            cam.fov = 1.0f;
        if (cam.fov > 45.0f)
            cam.fov = 45.0f;
    }

};

#endif