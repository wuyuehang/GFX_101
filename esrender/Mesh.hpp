#ifndef MESH_HPP
#define MESH_HPP

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Common.hpp"

class Mesh {
public:
    Mesh(const Mesh &) = delete;
    Mesh() {};
    ~Mesh() {};
    std::vector<Vertex> &get_vertices() { return m_vertices; }
    void load(const std::string, glm::mat4);
    glm::mat4 get_model_mat();

private:
    struct BoundingBox {
        float xmin;
        float xmax;
        float ymin;
        float ymax;
        float zmin;
        float zmax;
    };
    std::vector<Vertex> m_vertices;
    BoundingBox box;
    glm::mat4 m_model_mat;
};
#endif