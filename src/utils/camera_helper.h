#ifndef CAMERA_HELPER_H
#define CAMERA_HELPER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr float YAW = -90.0f;
constexpr float PITCH = 0.0f;
constexpr  float MovementSpeed = 100.5f;
constexpr  float MouseSensitivity = 0.1f;

class CameraHelper {
public:
    static void InitialCamera(CameraComponent& cam, glm::vec3 position, glm::vec3 up, float yaw, float pitch, Viewport vp)
    {
        cam.transform.position = position;
        cam.state.WorldUp = up;
        cam.yaw = yaw;
        cam.pitch = pitch;
        cam.viewport = vp;

        updateCameraVectors(cam);
    }


    static glm::mat4 getViewMatrix(CameraComponent& cam) {
        glm::vec3& position  = cam.transform.position;
        return glm::lookAt(position, position + cam.state.Front, cam.state.WorldUp);
    }

    static glm::mat4 getProjectionMatrix(CameraComponent& cam) {
        float aspectRatio = cam.viewport.width / cam.viewport.height;
        return glm::perspective(glm::radians(cam.fov), aspectRatio, cam.nearPlane, cam.farPlane);
    }

    static void updateCameraVectors(CameraComponent& cam)
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        front.y = sin(glm::radians(cam.pitch));
        front.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
        cam.state.Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        cam.state.Right = glm::normalize(glm::cross(cam.state.Front, cam.state.WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        cam.state.Up = glm::normalize(glm::cross(cam.state.Right, cam.state.Front));
    }


    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    static void ProcessMouseMovement(CameraComponent& cam, float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        cam.yaw += xoffset;
        cam.pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (cam.pitch > 89.0f)
                cam.pitch = 89.0f;
            if (cam.pitch < -89.0f)
                cam.pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors(cam);
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(CameraComponent& cam, float yoffset)
    {
        cam.fov -= yoffset;
        if (cam.fov < 1.0f)
            cam.fov = 1.0f;
        if (cam.fov > 45.0f)
            cam.fov = 45.0f;
    }

};

#endif