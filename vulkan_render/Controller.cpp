#include "Controller.hpp"
#include "HelloVulkan.hpp"
#include "trackball.h"

Controller::~Controller() {}

glm::mat4 FPSController::get_view() const { return glm::lookAt(eye, front + eye, up); }

void FPSController::handle_input() {
    static bool init_control_system = false;
    static float curr_time, last_time;
    float delta;
    static double curr_pitch, curr_yaw;
    static double xpos, ypos, last_xpos, last_ypos;
    static bool start = false;

    if (!init_control_system) {
        eye = glm::vec3(0.0, 0.0, 15.0);
        up = glm::vec3(0.0, 1.0, 0.0);
        front = glm::vec3(0.0, 0.0, -10000.0) - eye;

        fov = 45.0;
        ratio = 1.0;
        near = 0.1;
        far = 100.0;

        curr_pitch = 0.0;
        curr_yaw = 180.0; // looking at negative Z axis
        xpos = 0.0;
        ypos = 0.0;
        last_xpos = 0.0;
        last_ypos = 0.0;
        start = false;

        curr_time = 0.0;
        last_time = 0.0;
        init_control_system = true;
    }

    if (glfwGetKey(hello_vulkan->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(hello_vulkan->window, GLFW_TRUE);
    }

    {
        curr_time = glfwGetTime();
        delta = curr_time - last_time;
        last_time = curr_time;
    }

    float cameraSpeed = 2.75 * delta;
    float mouseSpeed = 2.75 * delta;

    if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwGetCursorPos(hello_vulkan->window, &xpos, &ypos);

        if (!start) { // when press first time, we update last x/y position only.
            last_xpos = xpos;
            last_ypos = ypos;
            start = true;
        } else { // when enter again, we calculate the yaw and pitch.
            curr_yaw += mouseSpeed * (xpos - last_xpos);
            curr_pitch += mouseSpeed * (ypos - last_ypos);
            last_xpos = xpos;
            last_ypos = ypos;
        }
    } else if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        start = false; // when release, we flag the ending.
    }

    front = glm::vec3(
        cos(glm::radians(curr_pitch)) * sin(glm::radians(curr_yaw)),
        sin(glm::radians(curr_pitch)),
        cos(glm::radians(curr_pitch)) * cos(glm::radians(curr_yaw)));

    if (glfwGetKey(hello_vulkan->window, GLFW_KEY_W) == GLFW_PRESS) {
        eye += cameraSpeed * glm::normalize(front);
    } else if (glfwGetKey(hello_vulkan->window, GLFW_KEY_S) == GLFW_PRESS) {
        eye -= cameraSpeed * glm::normalize(front);
    } else if (glfwGetKey(hello_vulkan->window, GLFW_KEY_A) == GLFW_PRESS) {
        eye -= cameraSpeed * glm::normalize(glm::cross(front, up));
    } else if (glfwGetKey(hello_vulkan->window, GLFW_KEY_D) == GLFW_PRESS) {
        eye += cameraSpeed * glm::normalize(glm::cross(front, up));
    } else if (glfwGetKey(hello_vulkan->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        init_control_system = false;
    }
}

glm::mat4 TrackballController::get_view() const { return glm::lookAt(eye, front, up); }

void TrackballController::handle_input() {
    static bool init_control_system = false;
    static double prevMouseX, prevMouseY;
    static bool mouseLeftPressed;
    static bool mouseMiddlePressed;
    static bool mouseRightPressed;

    if (!init_control_system) {
        trackball(curr_quat, 0, 0, 0, 0);

        eye = glm::vec3(0.0, 0.0, 3.0);
        up = glm::vec3(0.0, 1.0, 0.0);
        front = glm::vec3(0.0, 0.0, 0.0);

        fov = 30.0;
        ratio = hello_vulkan->w / (float)hello_vulkan->h;
        near = 0.1;
        far = 100.0;
        init_control_system = true;
    }

    if (glfwGetKey(hello_vulkan->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(hello_vulkan->window, GLFW_TRUE);
    }

    if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        mouseLeftPressed = true;
        trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
    } else if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        mouseLeftPressed = false;
    }

    if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        mouseRightPressed = true;
    } else if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        mouseRightPressed = false;
    }

    if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        mouseMiddlePressed = true;
    } else if (glfwGetMouseButton(hello_vulkan->window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE) {
        mouseMiddlePressed = false;
    }

    float rotScale = 1.0f;
    float transScale = 2.5f;
    double mouse_x, mouse_y;
    glfwGetCursorPos(hello_vulkan->window, &mouse_x, &mouse_y);
    const int & width = hello_vulkan->w;
    const int & height = hello_vulkan->h;

    if (mouseLeftPressed) {
        trackball(prev_quat, rotScale * (2.0f * prevMouseX - width) / (float)width,
            -rotScale * (height - 2.0f * prevMouseY) / (float)height, // negative sign flips Y
            rotScale * (2.0f * mouse_x - width) / (float)width,
            -rotScale * (height - 2.0f * mouse_y) / (float)height); // negative sign flips Y

        add_quats(prev_quat, curr_quat, curr_quat);
    } else if (mouseMiddlePressed) {
        eye[0] -= transScale * (mouse_x - prevMouseX) / (float)width;
        front[0] -= transScale * (mouse_x - prevMouseX) / (float)width;
        eye[1] -= transScale * (mouse_y - prevMouseY) / (float)height; // -= flips Y
        front[1] -= transScale * (mouse_y - prevMouseY) / (float)height; // -= flips Y
    } else if (mouseRightPressed) {
        eye[2] += transScale * (mouse_y - prevMouseY) / (float)height;
        front[2] += transScale * (mouse_y - prevMouseY) / (float)height;
    }

    // Update mouse point
    prevMouseX = mouse_x;
    prevMouseY = mouse_y;

    if (glfwGetKey(hello_vulkan->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        init_control_system = false;
    }
}
