#ifndef MESH_HPP
#define MESH_HPP

#include <GLES3/gl32.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Common.hpp"
#include "tiny_obj_loader.h"

class Mesh {
public:
    struct DrawObj {
        GLuint buffer_id;
        size_t material_id;
        std::vector<Vertex> vertices;
        Material material;
    };
    Mesh(const Mesh &) = delete;
    Mesh() {};
    ~Mesh() {};
    void load(const std::string, glm::mat4);
    glm::mat4 get_model_mat();
    std::vector<DrawObj> m_objects;
    std::vector<tinyobj::material_t> m_materials;
    std::map<std::string, GLuint> m_textures;
private:
    struct BoundingBox {
        float xmin;
        float xmax;
        float ymin;
        float ymax;
        float zmin;
        float zmax;
    };
    BoundingBox box;
    glm::mat4 m_model_mat;
};
#endif