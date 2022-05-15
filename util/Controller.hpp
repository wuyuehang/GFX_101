#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#if GL_BACKEND
#include <GL/glew.h>
#endif

#if ES_BACKEND
#include <GLES3/gl32.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "trackball.h"

namespace util {
class Controller {
public:
    virtual ~Controller() = 0;
    virtual void handle_input() = 0;
    virtual glm::mat4 get_model() = 0;
    virtual glm::mat4 get_view() const = 0;
    glm::vec3 get_eye() const { return eye; }
    float get_near() const { return near; }
    float get_far() const { return far; }
    glm::mat4 get_proj() const { return glm::perspective(glm::radians(fov), ratio, near, far); }

protected:
    struct init_config {
        glm::vec3 eye;
        glm::vec3 front;
        glm::vec3 up;
    };
    init_config m_init_config;
    glm::vec3 eye;
    glm::vec3 front;
    glm::vec3 up;
    float fov;
    float ratio;
    float near;
    float far;
};

class TrackballController : public Controller {
public:
    TrackballController() = delete;
    TrackballController(const TrackballController &) = delete;
    TrackballController(GLFWwindow *glfw, glm::vec3 eye_ = glm::vec3(0.0, 0.0, 3.0),
        glm::vec3 front_ = glm::vec3(0.0), glm::vec3 up_ = glm::vec3(0.0, 1.0, 0.0)) {
        m_init_config.eye = eye_;
        m_init_config.front = front_;
        m_init_config.up = up_;
        assert(glfw != nullptr);
        m_window = glfw;
        glfwGetWindowSize(m_window, &m_width, &m_height);
    }
    ~TrackballController() override {};
    void handle_input() override final;
    glm::mat4 get_view() const override final;
    glm::mat4 get_model() {
        float rotation[16];
        build_rotmatrix((float (*)[4])rotation, curr_quat);
        return glm::make_mat4(rotation);
    }

private:
    GLFWwindow *m_window;
    int m_width;
    int m_height;
    float curr_quat[4];
    float prev_quat[4];
};

// fix view transform.
// rotate skybox by x/y axis to give illusion that it is from "FPS" angle.
// it is used for debug purpose to explore inside the skybox
class SkyboxController : public Controller {
public:
    SkyboxController() = delete;
    SkyboxController(const SkyboxController &) = delete;
    SkyboxController(GLFWwindow *glfw, glm::vec3 eye_ = glm::vec3(0.0, 0.0, 3.0),
        glm::vec3 front_ = glm::vec3(0.0), glm::vec3 up_ = glm::vec3(0.0, 1.0, 0.0)) {
        m_init_config.eye = eye_;
        m_init_config.front = front_;
        m_init_config.up = up_;
        assert(glfw != nullptr);
        m_window = glfw;
        glfwGetWindowSize(m_window, &m_width, &m_height);
    }
    ~SkyboxController() override {};
    void handle_input() override final;
    glm::mat4 get_view() const override final;
    glm::mat4 get_model() {
        // rotate horizon
        glm::mat4 rot_y = glm::rotate(glm::mat4(1.0), glm::radians(curr_x_angle), glm::vec3(0.0, 1.0, 0.0));
        // rotate vertical
        glm::mat4 rot_x = glm::rotate(glm::mat4(1.0), glm::radians(curr_y_angle), glm::vec3(1.0, 0.0, 0.0));
        return rot_x * rot_y;
    }

private:
    GLFWwindow *m_window;
    int m_width;
    int m_height;
    float curr_x_angle;
    float curr_y_angle;
};
}

#endif
