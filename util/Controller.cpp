#include "Controller.hpp"
#include "trackball.h"

namespace util {
Controller::~Controller() {}

glm::mat4 TrackballController::get_view() const { return glm::lookAt(eye, front, up); }

void TrackballController::handle_input() {
    static bool init_control_system = false;
    static double prevMouseX, prevMouseY;
    static bool mouseLeftPressed;
    static bool mouseMiddlePressed;
    static bool mouseRightPressed;

    if (!init_control_system) {
        trackball(curr_quat, 0, 0, 0, 0);

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
        trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
    } else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        mouseLeftPressed = false;
    }

    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        mouseRightPressed = true;
    } else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        mouseRightPressed = false;
    }

    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        mouseMiddlePressed = true;
    } else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) {
        mouseMiddlePressed = false;
    }

    float rotScale = 1.0f;
    float transScale = 2.5f;
    double mouse_x, mouse_y;
    glfwGetCursorPos(m_window, &mouse_x, &mouse_y);
    const int & width = m_width;
    const int & height = m_height;

    if (mouseLeftPressed) {
        trackball(prev_quat, rotScale * (2.0f * prevMouseX - width) / (float)width,
            rotScale * (height - 2.0f * prevMouseY) / (float)height,
            rotScale * (2.0f * mouse_x - width) / (float)width,
            rotScale * (height - 2.0f * mouse_y) / (float)height);

        add_quats(prev_quat, curr_quat, curr_quat);
    } else if (mouseMiddlePressed) {
        eye[0] -= transScale * (mouse_x - prevMouseX) / (float)width;
        front[0] -= transScale * (mouse_x - prevMouseX) / (float)width;
        eye[1] += transScale * (mouse_y - prevMouseY) / (float)height;
        front[1] += transScale * (mouse_y - prevMouseY) / (float)height;
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
