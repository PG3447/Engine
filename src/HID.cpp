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

void HID::update() {
    //to czy obecnie klawisz jest klikniety i czy wlasnie nie byl puszczony
    for (auto& [key, state] : input_keys) {
        state.previous = state.current;
        state.current = glfwGetKey(window, key);
    }

    glfwGetCursorPos(window, &mouse_x, &mouse_y); // myszka

    scroll_y = 0.0; //scroll
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
    return false;
}

void HID::scroll_callback(GLFWwindow *, double, double yoffset) {
    HID::get().scroll_y += yoffset;
}


