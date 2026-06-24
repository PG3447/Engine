bool processCameraInput(ECS& ecs, CameraComponent& cam, TransformComponent& playerTransform,
    const std::string& up,
    const std::string& down,
    const std::string& left,
    const std::string& right)
{
    const auto& hid = ecs.GetSystem<HID>();
    glm::vec3 dir(0.0f);

    glm::vec3 camFront = cam.state.Front;
    camFront.y = 0.0f;
    camFront = glm::normalize(camFront);

    glm::vec3 camRight = cam.state.Right;
    camRight.y = 0.0f;
    camRight = glm::normalize(camRight);

    if (hid->is_action_pressed(up))    dir += camFront;
    if (hid->is_action_pressed(down))  dir -= camFront;
    if (hid->is_action_pressed(left))  dir -= camRight;
    if (hid->is_action_pressed(right)) dir += camRight;

    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        playerTransform.position += dir * MovementSpeed * 0.04f; //deltaTime; aktualnie fixedDeltaTime
        playerTransform.isDirty = true;
        cam.dirty = true;
        return true;
    }
    return false;
}


const float sensitivityCamera = 1200.0f;

bool processCameraMouse(ECS& ecs, CameraComponent& cam, TransformComponent& transformCamera, TransformComponent& playerTransform)
{
    const auto& hid = ecs.GetSystem<HID>();
    float dx = hid->get_mouse_dx();
    float dy = hid->get_mouse_dy();
    const float epsilon = 0.01f;
    if (glm::abs(dx) < epsilon && glm::abs(dy) < epsilon)
        return false;

    playerTransform.rotation.y -= dx * sensitivityCamera / 600.0f * 0.04f;
    playerTransform.isDirty = true;
    CameraHelper::ProcessMouseMovement(cam, transformCamera, 0.0f, dy);

    return glm::abs(dx) >= epsilon;
}

float lookDeadzone = 0.0f;

bool processCameraGamepad(ECS& ecs, CameraComponent& cam, TransformComponent& transformCamera, TransformComponent& playerTransform, int gamepad_id, bool& outIsTurning)
{
    const auto& hid = ecs.GetSystem<HID>();
    float lx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_X, gamepad_id);
    float ly = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_LEFT_Y, gamepad_id);

    glm::vec3 dir(0.0f);
    glm::vec3 camFront = cam.state.Front;
    camFront.y = 0.0f;
    camFront = glm::normalize(camFront);

    glm::vec3 camRight = cam.state.Right;
    camRight.y = 0.0f;
    camRight = glm::normalize(camRight);

    dir += camFront * (-ly);
    dir += camRight * lx;

    bool isMoving = false;
    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
        playerTransform.position += dir * MovementSpeed * 0.04f;
        playerTransform.isDirty = true;
        cam.dirty = true;
        isMoving = true;
    }

    float rx = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_X, gamepad_id);
    float ry = hid->get_gamepad_axis(GLFW_GAMEPAD_AXIS_RIGHT_Y, gamepad_id);
    if (lookDeadzone <= 0.01f)
        lookDeadzone += 0.0005f;
    else if (glm::abs(rx) < lookDeadzone && glm::abs(ry) < lookDeadzone) {
        outIsTurning = false;
        return isMoving;
    }

    outIsTurning = (glm::abs(rx) >= lookDeadzone);
    playerTransform.rotation.y -= rx * sensitivityCamera / 10.0f * deltaTime;
    playerTransform.isDirty = true;
    CameraHelper::ProcessMouseMovement(cam, transformCamera, 0.0f, ry * sensitivityCamera * deltaTime);
    return isMoving;
}

