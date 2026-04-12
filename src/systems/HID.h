#ifndef OPENGLGP_HID_H
#define OPENGLGP_HID_H

#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/ecs.h"

class HID : public System{
private:
    struct KeyState {
        bool current = false;
        bool previous = false;
    };

    GLFWwindow* window = nullptr;

    std::unordered_map<std::string, std::vector<int>> action_name_map;
    std::unordered_map<int, KeyState> input_keys;

    std::unordered_map<std::string, std::vector<int>> action_name_mouse_map;
    std::unordered_map<int, KeyState> mouse_buttons;

    static const int MAX_GAMEPADS = 2;
    std::unordered_map<std::string, std::vector<std::pair<int,int>>> action_name_gamepad_map;
    KeyState gamepad_buttons[MAX_GAMEPADS][GLFW_GAMEPAD_BUTTON_LAST + 1] = {};

    double mouse_x = 0;
    double mouse_y = 0;
    double scroll_y = 0;
    double mouse_dx = 0;
    double mouse_dy = 0;
    double mouse_prev_x = 0;
    double mouse_prev_y = 0;

    static void scroll_callback(GLFWwindow*, double, double yoffset);

public:
    HID(ECS& ecs, GLFWwindow* win) : window(win)
    {
        init(win);
    }

    void Update(ECS &ecs) override;

    void init(GLFWwindow* window);

    void name_action(const std::string action_name, int glfw_key);
    void name_action_mouse(const std::string action_name, int glfw_key);
    void name_action_gamepad(const std::string action_name, int glfw_key, int gamepad_id);

    bool is_action_pressed(const std::string& action_name);
    bool is_action_just_pressed(const std::string& action_name);
    bool is_action_just_released(const std::string& action_name);

    float get_gamepad_axis(int glfw_axis, int gamepad_id = 0) const;

    double get_mouse_x() const {return mouse_x;};
    double get_mouse_y() const {return mouse_y;};
    double get_mouse_dx()  const { return mouse_dx; }
    double get_mouse_dy()  const { return mouse_dy; }
    double get_scroll() const {return scroll_y;};

    bool is_gamepad_connected(int gamepad_id = 0) const;

    void OnGameObjectUpdated(GameObject *e) override;//unused
};


#endif //OPENGLGP_HID_H