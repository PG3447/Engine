#include "HID.h"
#include <spdlog/spdlog.h>


void HID::init(GLFWwindow *_window) {
    window = _window;
    if (!window) {
        spdlog::error("Window is null.");
        return;
    }
    load_inputs_from_yaml("res/yaml/input_map.yaml");
    glfwSetWindowUserPointer(window, this);
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

void HID::load_inputs_from_yaml(const std::string& path) {
    YamlConfig config;
    if (!config.load(path)) {
        spdlog::error("HID: can't load inputs from: {}", path);
        return;
    }

    //na wszelki wypadek jest czyszczone
    action_name_map.clear();
    input_keys.clear();
    action_name_mouse_map.clear();
    mouse_buttons.clear();
    action_name_gamepad_map.clear();

    static const std::unordered_map<std::string, int> key_map = {
        // klawisze literowe
        {"GLFW_KEY_A", GLFW_KEY_A}, {"GLFW_KEY_B", GLFW_KEY_B}, {"GLFW_KEY_C", GLFW_KEY_C},
        {"GLFW_KEY_D", GLFW_KEY_D}, {"GLFW_KEY_E", GLFW_KEY_E}, {"GLFW_KEY_F", GLFW_KEY_F},
        {"GLFW_KEY_G", GLFW_KEY_G}, {"GLFW_KEY_H", GLFW_KEY_H}, {"GLFW_KEY_I", GLFW_KEY_I},
        {"GLFW_KEY_J", GLFW_KEY_J}, {"GLFW_KEY_K", GLFW_KEY_K}, {"GLFW_KEY_L", GLFW_KEY_L},
        {"GLFW_KEY_M", GLFW_KEY_M}, {"GLFW_KEY_N", GLFW_KEY_N}, {"GLFW_KEY_O", GLFW_KEY_O},
        {"GLFW_KEY_P", GLFW_KEY_P}, {"GLFW_KEY_Q", GLFW_KEY_Q}, {"GLFW_KEY_R", GLFW_KEY_R},
        {"GLFW_KEY_S", GLFW_KEY_S}, {"GLFW_KEY_T", GLFW_KEY_T}, {"GLFW_KEY_U", GLFW_KEY_U},
        {"GLFW_KEY_V", GLFW_KEY_V}, {"GLFW_KEY_W", GLFW_KEY_W}, {"GLFW_KEY_X", GLFW_KEY_X},
        {"GLFW_KEY_Y", GLFW_KEY_Y}, {"GLFW_KEY_Z", GLFW_KEY_Z},
        // cyfry
        {"GLFW_KEY_0", GLFW_KEY_0}, {"GLFW_KEY_1", GLFW_KEY_1}, {"GLFW_KEY_2", GLFW_KEY_2},
        {"GLFW_KEY_3", GLFW_KEY_3}, {"GLFW_KEY_4", GLFW_KEY_4}, {"GLFW_KEY_5", GLFW_KEY_5},
        {"GLFW_KEY_6", GLFW_KEY_6}, {"GLFW_KEY_7", GLFW_KEY_7}, {"GLFW_KEY_8", GLFW_KEY_8},
        {"GLFW_KEY_9", GLFW_KEY_9},
        // strzalki
        {"GLFW_KEY_UP", GLFW_KEY_UP}, {"GLFW_KEY_DOWN", GLFW_KEY_DOWN},
        {"GLFW_KEY_LEFT", GLFW_KEY_LEFT}, {"GLFW_KEY_RIGHT", GLFW_KEY_RIGHT},
        // inne
        {"GLFW_KEY_SPACE", GLFW_KEY_SPACE}, {"GLFW_KEY_ENTER", GLFW_KEY_ENTER},
        {"GLFW_KEY_ESCAPE", GLFW_KEY_ESCAPE}, {"GLFW_KEY_TAB", GLFW_KEY_TAB},
        {"GLFW_KEY_LEFT_SHIFT", GLFW_KEY_LEFT_SHIFT}, {"GLFW_KEY_RIGHT_SHIFT", GLFW_KEY_RIGHT_SHIFT},
        {"GLFW_KEY_LEFT_CONTROL", GLFW_KEY_LEFT_CONTROL}, {"GLFW_KEY_RIGHT_CONTROL", GLFW_KEY_RIGHT_CONTROL},
        {"GLFW_KEY_LEFT_ALT", GLFW_KEY_LEFT_ALT}, {"GLFW_KEY_RIGHT_ALT", GLFW_KEY_RIGHT_ALT},
        {"GLFW_KEY_BACKSPACE", GLFW_KEY_BACKSPACE}, {"GLFW_KEY_DELETE", GLFW_KEY_DELETE},
        {"GLFW_KEY_INSERT", GLFW_KEY_INSERT}, {"GLFW_KEY_HOME", GLFW_KEY_HOME},
        {"GLFW_KEY_END", GLFW_KEY_END}, {"GLFW_KEY_PAGE_UP", GLFW_KEY_PAGE_UP},
        {"GLFW_KEY_PAGE_DOWN", GLFW_KEY_PAGE_DOWN},
        // Fy
        {"GLFW_KEY_F1", GLFW_KEY_F1}, {"GLFW_KEY_F2", GLFW_KEY_F2}, {"GLFW_KEY_F3", GLFW_KEY_F3},
        {"GLFW_KEY_F4", GLFW_KEY_F4}, {"GLFW_KEY_F5", GLFW_KEY_F5}, {"GLFW_KEY_F6", GLFW_KEY_F6},
        {"GLFW_KEY_F7", GLFW_KEY_F7}, {"GLFW_KEY_F8", GLFW_KEY_F8}, {"GLFW_KEY_F9", GLFW_KEY_F9},
        {"GLFW_KEY_F10", GLFW_KEY_F10}, {"GLFW_KEY_F11", GLFW_KEY_F11}, {"GLFW_KEY_F12", GLFW_KEY_F12},
        // myszka
        {"GLFW_MOUSE_BUTTON_LEFT", GLFW_MOUSE_BUTTON_LEFT},
        {"GLFW_MOUSE_BUTTON_RIGHT", GLFW_MOUSE_BUTTON_RIGHT},
        {"GLFW_MOUSE_BUTTON_MIDDLE", GLFW_MOUSE_BUTTON_MIDDLE},
        // gamepad przyciski
        {"GLFW_GAMEPAD_BUTTON_A", GLFW_GAMEPAD_BUTTON_A},
        {"GLFW_GAMEPAD_BUTTON_B", GLFW_GAMEPAD_BUTTON_B},
        {"GLFW_GAMEPAD_BUTTON_X", GLFW_GAMEPAD_BUTTON_X},
        {"GLFW_GAMEPAD_BUTTON_Y", GLFW_GAMEPAD_BUTTON_Y},
        {"GLFW_GAMEPAD_BUTTON_SQUARE", GLFW_GAMEPAD_BUTTON_SQUARE},
        {"GLFW_GAMEPAD_BUTTON_CROSS", GLFW_GAMEPAD_BUTTON_CROSS},
        {"GLFW_GAMEPAD_BUTTON_CIRCLE", GLFW_GAMEPAD_BUTTON_CIRCLE},
        {"GLFW_GAMEPAD_BUTTON_TRIANGLE", GLFW_GAMEPAD_BUTTON_TRIANGLE},
        {"GLFW_GAMEPAD_BUTTON_LEFT_BUMPER", GLFW_GAMEPAD_BUTTON_LEFT_BUMPER},
        {"GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER},
        {"GLFW_GAMEPAD_BUTTON_BACK", GLFW_GAMEPAD_BUTTON_BACK},
        {"GLFW_GAMEPAD_BUTTON_START", GLFW_GAMEPAD_BUTTON_START},
        {"GLFW_GAMEPAD_BUTTON_DPAD_UP", GLFW_GAMEPAD_BUTTON_DPAD_UP},
        {"GLFW_GAMEPAD_BUTTON_DPAD_RIGHT", GLFW_GAMEPAD_BUTTON_DPAD_RIGHT},
        {"GLFW_GAMEPAD_BUTTON_DPAD_DOWN", GLFW_GAMEPAD_BUTTON_DPAD_DOWN},
        {"GLFW_GAMEPAD_BUTTON_DPAD_LEFT", GLFW_GAMEPAD_BUTTON_DPAD_LEFT},
    };

    auto resolve_key = [&](const std::string& name) -> std::optional<int> {
        auto it = key_map.find(name);
        if (it == key_map.end()) {
            spdlog::warn("HID: unknown button '{}'", name);
            return std::nullopt;
        }
        return it->second;
    };

    YAML::Node root = config.getRoot();
    YAML::Node inputs = root["input"];

    if (!inputs) {
        spdlog::warn("HID: file {} has no 'input' sction", path);
        return;
    }

    if (inputs["keyboard"]) {
        for (auto action_node : inputs["keyboard"]) {
            std::string action = action_node.first.as<std::string>();
            for (auto key_node : action_node.second) {
                auto key = resolve_key(key_node.as<std::string>());
                if (key) name_action(action, *key);
            }
        }
    }

    if (inputs["mouse"]) {
        for (auto action_node : inputs["mouse"]) {
            std::string action = action_node.first.as<std::string>();
            for (auto btn_node : action_node.second) {
                auto btn = resolve_key(btn_node.as<std::string>());
                if (btn) name_action_mouse(action, *btn);
            }
        }
    }

    if (inputs["gamepad"]) {
        for (auto action_node : inputs["gamepad"]) {
            std::string action = action_node.first.as<std::string>();
            for (auto entry : action_node.second) {
                if (!entry["button"] || !entry["id"]) {
                    spdlog::warn("HID: gamepad action {} cannot be loaded (no id or button section in yaml)", action);
                    continue;
                }
                auto btn = resolve_key(entry["button"].as<std::string>());
                int id = entry["id"].as<int>();
                if (btn) name_action_gamepad(action, *btn, id);
            }
        }
    }

    spdlog::info("HID: input map loaded from {}", path);
}

void HID::Update(ECS& ecs) {
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
    if (it != action_name_map.end())
    {
        for (int key : it->second) {
            auto key_state = input_keys.find(key);
            if (key_state != input_keys.end() && key_state->second.current) return true;
        }
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


bool HID::is_action_just_pressed(const std::string &action_name)
{
    auto it = action_name_map.find(action_name);
    if (it != action_name_map.end()){
        for (int key : it->second) {
            auto key_state = input_keys.find(key);
            if (key_state != input_keys.end() && key_state->second.current && !key_state->second.previous)
                return true;
        }
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
    if (it != action_name_map.end())
    {
        for (int key : it->second) {
            auto key_state = input_keys.find(key);
            if (key_state != input_keys.end() && !key_state->second.current && key_state->second.previous)
                return true;
        }
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

void HID::OnGameObjectUpdated(GameObject *e) {
    //kill c++ creators
}

void HID::scroll_callback(GLFWwindow *window, double, double yoffset) {
    auto* hid = static_cast<HID*>(glfwGetWindowUserPointer(window));
    hid->scroll_y += yoffset;
}


