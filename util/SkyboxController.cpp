#include "Controller.hpp"

namespace util {

glm::mat4 SkyboxController::get_view() const { return glm::lookAt(eye, front, up); }

void SkyboxController::handle_input() {
    static bool init_control_system = false;
    static double prevMouseX, prevMouseY;
    static bool mouseLeftPressed;
    static bool mouseRightPressed;
    static float curr_time, last_time;

    if (!init_control_system) {
        curr_x_angle = 0.0;
        curr_y_angle = 0.0;
        curr_time = 0.0;
        last_time = 0.0;

        eye = m_init_config.eye;
        front = m_init_config.front;
        up = m_init_config.up;

        fov = 30.0;
        ratio = m_width / (float)m_height;
        near = 0.1;
        far = 100.0;
        init_control_system = true;
    }

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        mouseLeftPressed = true;
    } else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        mouseLeftPressed = false;
    }

    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        mouseRightPressed = true;
    } else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        mouseRightPressed = false;
    }

    float delta;
    curr_time = glfwGetTime();
    delta = curr_time - last_time;
    last_time = curr_time;
    float rotScale = 2.75f * delta;
    float transScale = 2.5f;
    double mouse_x, mouse_y;
    glfwGetCursorPos(m_window, &mouse_x, &mouse_y);
    const int & height = m_height;

    if (mouseLeftPressed) {
        if (curr_x_angle == 0.0 && curr_y_angle == 0.0) {
            curr_x_angle = 1e-10; // just a trick
        } else {
            if (std::abs(mouse_x - prevMouseX) >= std::abs(mouse_y - prevMouseY)) {
                curr_x_angle += rotScale * (mouse_x - prevMouseX) * ratio;
            } else {
                curr_y_angle += rotScale * (mouse_y - prevMouseY);
            }
        }
    } else if (mouseRightPressed) {
        eye[2] += transScale * (mouse_y - prevMouseY) / (float)height;
        front[2] += transScale * (mouse_y - prevMouseY) / (float)height;
    }

    // Update mouse point
    prevMouseX = mouse_x;
    prevMouseY = mouse_y;

    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        init_control_system = false;
    }
}
}