#include "HID.h"
#include <spdlog/spdlog.h>


HID & HID::get() {
    static HID instance;
    return instance;
}

void HID::init(GLFWwindow *_window) {
    window = _window;
    if (!window) {
        spdlog::error("Window is null.");
        return;
    }
    glfwSetScrollCallback(window, HID::scroll_callback);
}

void HID::name_action(const std::string action_name, int glfw_key) {
    action_name_map[action_name].push_back(glfw_key);
    input_keys.try_emplace(glfw_key);
}

void HID::name_action_mouse(const std::string action_name, int glfw_key) {
    action_name_mouse_map[action_name].push_back(glfw_key);
    mouse_buttons.try_emplace(glfw_key);
}

void HID::name_action_gamepad(const std::string action_name, int glfw_key, int gamepad_id) {
    action_name_gamepad_map[action_name].push_back({glfw_key, gamepad_id});
}

void HID::update() {
    //to czy obecnie klawisz jest klikniety i czy wlasnie nie byl puszczony
    for (auto& [key, state] : input_keys) {
        state.previous = state.current;
        state.current = glfwGetKey(window, key);
    }

    for (auto& [btn, state] : mouse_buttons) {
        state.previous = state.current;
        state.current  = (glfwGetMouseButton(window, btn) == GLFW_PRESS);
    }

    mouse_prev_x = mouse_x;
    mouse_prev_y = mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    mouse_dx = mouse_x - mouse_prev_x;
    mouse_dy = mouse_y - mouse_prev_y;

    scroll_y = 0.0; //scroll

    for (int id = 0; id < MAX_GAMEPADS; ++id) {
        if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + id)) continue;

        GLFWgamepadstate state;
        if (!glfwGetGamepadState(GLFW_JOYSTICK_1 + id, &state)) continue;

        for (int btn = 0; btn <= GLFW_GAMEPAD_BUTTON_LAST; ++btn) {
            gamepad_buttons[id][btn].previous = gamepad_buttons[id][btn].current;
            gamepad_buttons[id][btn].current  = (state.buttons[btn] == GLFW_PRESS);
        }
    }
}


bool HID::is_action_pressed(const std::string &action_name) {
    auto it = action_name_map.find(action_name);
    if (it == action_name_map.end()) {
        return false;
    }
    for (int key : it->second) {
        auto key_state = input_keys.find(key);
        if (key_state != input_keys.end() && key_state->second.current) return true;
    }
    auto mit = action_name_mouse_map.find(action_name);
    if (mit != action_name_mouse_map.end()) {
        for (int btn : mit->second) {
            auto key_state = mouse_buttons.find(btn);
            if (key_state != mouse_buttons.end() && key_state->second.current) return true;
        }
    }
    auto git = action_name_gamepad_map.find(action_name);
    if (git != action_name_gamepad_map.end()) {
        for (auto [btn, id] : git->second) {
            if (gamepad_buttons[id][btn].current) return true;
        }
    }

    return false;
}


bool HID::is_action_just_pressed(const std::string &action_name) {
    auto it = action_name_map.find(action_name);
    if (it == action_name_map.end()) return false;
    for (int key : it->second) {
        auto key_state = input_keys.find(key);
        if (key_state != input_keys.end() && key_state->second.current && !key_state->second.previous)
            return true;
    }
    auto mit = action_name_mouse_map.find(action_name);
    if (mit != action_name_mouse_map.end()) {
        for (int btn : mit->second) {
            auto key_state = mouse_buttons.find(btn);
            if (key_state != mouse_buttons.end() && key_state->second.current && !key_state->second.previous) return true;
        }
    }

    auto git = action_name_gamepad_map.find(action_name);
    if (git != action_name_gamepad_map.end()) {
        for (auto [btn, id] : git->second) {
            if (gamepad_buttons[id][btn].current && !gamepad_buttons[id][btn].previous) return true;
        }
    }

    return false;
}


bool HID::is_action_just_released(const std::string &action_name) {
    auto it = action_name_map.find(action_name);
    if (it == action_name_map.end()) return false;
    for (int key : it->second) {
        auto key_state = input_keys.find(key);
        if (key_state != input_keys.end() && !key_state->second.current && key_state->second.previous)
            return true;
    }
    auto mit = action_name_mouse_map.find(action_name);
    if (mit != action_name_mouse_map.end()) {
        for (int btn : mit->second) {
            auto key_state = mouse_buttons.find(btn);
            if (key_state != mouse_buttons.end() && !key_state->second.current && key_state->second.previous) return true;
        }
    }

    auto git = action_name_gamepad_map.find(action_name);
    if (git != action_name_gamepad_map.end()) {
        for (auto [btn, id] : git->second) {
            if (!gamepad_buttons[id][btn].current && gamepad_buttons[id][btn].previous) return true;
        }
    }
    return false;
}

float HID::get_gamepad_axis(int glfw_axis, int gamepad_id) const {
    if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + gamepad_id)) return 0.0f;

    GLFWgamepadstate state;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1 + gamepad_id, &state)) return 0.0f;

    float value = state.axes[glfw_axis];

    const float deadzone = 0.2f;
    if (value > -deadzone && value < deadzone) return 0.0f;

    return value;
}

void HID::scroll_callback(GLFWwindow *, double, double yoffset) {
    HID::get().scroll_y += yoffset;
}


