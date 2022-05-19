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

struct AdvVertex {
    glm::vec3 pos;
    glm::vec3 nor;
    glm::vec2 uv;
    glm::vec3 tan; // tangent
    glm::vec3 bta; // bitangent
};

struct Material {
    glm::vec3 Ka; // ambient
    glm::vec3 Kd; // diffuse
    glm::vec3 Ks; // specular
};

// Range, Kc, Kl, Kq
// 3250, 1.0, 0.0014, 0.000007
// 600, 1.0, 0.007, 0.0002
// 325, 1.0, 0.014, 0.0007
// 200, 1.0, 0.022, 0.0019
// 160, 1.0, 0.027, 0.0028
// 100, 1.0, 0.045, 0.0075
// 65, 1.0, 0.07, 0.017
// 50, 1.0, 0.09, 0.032 (f.inst light disapears @ 50)
// 32, 1.0, 0.14, 0.07
// 20, 1.0, 0.22, 0.20
// 13, 1.0, 0.35, 0.44
// 7, 1.0, 0.7, 1.8
// Factor = 1.0 / (Kc + Kl*d + Kq*d*d)
struct Attenuation {
    float Kc;
    float Kl;
    float Kq;
};
#endif
