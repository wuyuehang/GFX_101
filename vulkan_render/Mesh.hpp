#ifndef MESH_HPP
#define MESH_HPP

#include <glm/glm.hpp>
#include <string>
#include <vector>

class Mesh {
public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 nor;
        glm::vec2 uv;
    };
    Mesh(const Mesh &) = delete;
    Mesh() {};
    ~Mesh() {};
    std::vector<Vertex> &get_vertices() { return m_vertices; } 
    void load(const std::string);

private:
    std::vector<Vertex> m_vertices;
};
#endif