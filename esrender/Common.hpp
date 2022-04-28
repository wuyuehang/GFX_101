#ifndef COMMON_HPP
#define COMMON_HPP

#include <glm/glm.hpp>

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 nor;
    glm::vec2 uv;
};

#endif