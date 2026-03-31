#ifndef OPENGLGP_HID_H
#define OPENGLGP_HID_H

#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <vector>

class HID {
private:
    HID() = default;
    ~HID() = default;

    struct KeyState {
        bool current = false;
        bool previous = false;
    };

    GLFWwindow* window = nullptr;

    std::unordered_map<std::string, std::vector<int>> action_name_map;

    std::unordered_map<int, KeyState> input_keys;

    double mouse_x = 0;
    double mouse_y = 0;
    double scroll_y = 0;

    static void scroll_callback(GLFWwindow*, double, double yoffset);

public:
    static HID& get();

    void init(GLFWwindow* window);

    void update();

    void name_action(const std::string action_name, int glfw_key);

    bool is_action_pressed(const std::string& action_name);
    bool is_action_just_pressed(const std::string& action_name);
    bool is_action_just_released(const std::string& action_name);

    double get_mouse_x() const {return mouse_x;};
    double get_mouse_y() const {return mouse_y;};
    double get_scroll() const {return scroll_y;};
};


#endif //OPENGLGP_HID_H