#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "trackball.h"

class GLFWwindow;

namespace util {
class Controller {
public:
    virtual ~Controller() = 0;
    virtual void handle_input() = 0;
    virtual glm::mat4 get_model() = 0;
    virtual glm::mat4 get_view() const = 0;
    glm::vec3 get_eye() const { return eye; }
    glm::mat4 get_proj() const { return glm::perspective(glm::radians(fov), ratio, near, far); }

protected:
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
    TrackballController(GLFWwindow *glfw);
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
}

#endif
