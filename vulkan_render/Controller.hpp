#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cassert>
#include "trackball.h"
class HelloVulkan;

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

class FPSController : public Controller {
public:
    FPSController() = delete;
    FPSController(const FPSController &) = delete;
    FPSController(HelloVulkan *app) : hello_vulkan(app) { assert(app != nullptr); }
    ~FPSController() override {};
    void handle_input() override final;
    glm::mat4 get_model() override final { return glm::mat4(1.0); }
    glm::mat4 get_view() const override final;
private:
    const HelloVulkan *hello_vulkan;
};

class TrackballController : public Controller {
public:
    TrackballController() = delete;
    TrackballController(const TrackballController &) = delete;
    TrackballController(HelloVulkan *app) : hello_vulkan(app) { assert(app != nullptr); }
    ~TrackballController() override {};
    void handle_input() override final;
    glm::mat4 get_view() const override final;
    glm::mat4 get_model() {
        float rotation[16];
        build_rotmatrix((float (*)[4])rotation, curr_quat);
        return glm::make_mat4(rotation);
    }

private:
    const HelloVulkan *hello_vulkan;
    float curr_quat[4];
    float prev_quat[4];
};

#endif
