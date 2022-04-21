#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "tiny_obj_loader.h"
#include "Mesh.hpp"

void Mesh::load(const std::string filename, glm::mat4 pre_rotation) {
/*
    attrib_t::vertices => 3 floats per vertex

        v[0]        v[1]        v[2]        v[3]               v[n-1]
    +-----------+-----------+-----------+-----------+      +-----------+
    | x | y | z | x | y | z | x | y | z | x | y | z | .... | x | y | z |
    +-----------+-----------+-----------+-----------+      +-----------+

    attrib_t::normals => 3 floats per vertex

        n[0]        n[1]        n[2]        n[3]               n[n-1]
    +-----------+-----------+-----------+-----------+      +-----------+
    | x | y | z | x | y | z | x | y | z | x | y | z | .... | x | y | z |
    +-----------+-----------+-----------+-----------+      +-----------+

    attrib_t::texcoords => 2 floats per vertex

        t[0]        t[1]        t[2]        t[3]               t[n-1]
    +-----------+-----------+-----------+-----------+      +-----------+
    |  u  |  v  |  u  |  v  |  u  |  v  |  u  |  v  | .... |  u  |  v  |
    +-----------+-----------+-----------+-----------+      +-----------+

    attrib_t::colors => 3 floats per vertex(vertex color. optional)

        c[0]        c[1]        c[2]        c[3]               c[n-1]
    +-----------+-----------+-----------+-----------+      +-----------+
    | x | y | z | x | y | z | x | y | z | x | y | z | .... | x | y | z |
    +-----------+-----------+-----------+-----------+      +-----------+
*/
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
        assert(0);
    }

    std::cout << "vertice  = " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "normal   = " << attrib.normals.size() / 3 << std::endl;
    std::cout << "uv       = " << attrib.texcoords.size() / 2 << std::endl;
    std::cout << "material = " << materials.size() << std::endl;
    std::cout << "shape    = " << shapes.size() << std::endl;

    box.xmax = box.ymax = box.zmax = -std::numeric_limits<float>::max();
    box.xmin = box.ymin = box.zmin = std::numeric_limits<float>::max();

    for (const auto& shape : shapes) {
        // Faces are defined using lists of vertex, texture and normal indices in the format vertex_index/texture_index/normal_index
        // f 3/3/1 2/2/1 1/1/1
        // f 1/1/1 4/4/1 3/3/1
        // assume a face is a triangle
        for (auto f = 0; f < shape.mesh.indices.size() / 3; f++) {
            tinyobj::index_t v0 = shape.mesh.indices[3 * f + 0];
            tinyobj::index_t v1 = shape.mesh.indices[3 * f + 1];
            tinyobj::index_t v2 = shape.mesh.indices[3 * f + 2];

            // TODO take care of vertex order
            glm::vec3 manual_normal;
            if (attrib.normals.size() <= 0) {
                glm::vec3 A = glm::vec3(attrib.vertices[3 * v0.vertex_index + 0], attrib.vertices[3 * v0.vertex_index + 1], attrib.vertices[3 * v0.vertex_index + 2]);
                glm::vec3 B = glm::vec3(attrib.vertices[3 * v1.vertex_index + 0], attrib.vertices[3 * v1.vertex_index + 1], attrib.vertices[3 * v1.vertex_index + 2]);
                glm::vec3 C = glm::vec3(attrib.vertices[3 * v2.vertex_index + 0], attrib.vertices[3 * v2.vertex_index + 1], attrib.vertices[3 * v2.vertex_index + 2]);
                manual_normal = - glm::cross(C-A, B-A);
                manual_normal = glm::normalize(manual_normal);
            }
            Vertex temp {};
            // process first vertex
            temp.pos = {
                attrib.vertices[3 * v0.vertex_index + 0],
                attrib.vertices[3 * v0.vertex_index + 1],
                attrib.vertices[3 * v0.vertex_index + 2],
            };

            if (attrib.normals.size() > 0) {
                temp.nor = {
                    attrib.normals[3 * v0.normal_index + 0],
                    attrib.normals[3 * v0.normal_index + 1],
                    attrib.normals[3 * v0.normal_index + 2],
                };
            } else {
                temp.nor = manual_normal;
            }

            temp.uv = {
                attrib.texcoords[2 * v0.texcoord_index + 0],
                1.0 - attrib.texcoords[2 * v0.texcoord_index + 1], // need flip UV for vulkan
            };

            m_vertices.push_back(temp);
            // process second vertex
            temp.pos = {
                attrib.vertices[3 * v1.vertex_index + 0],
                attrib.vertices[3 * v1.vertex_index + 1],
                attrib.vertices[3 * v1.vertex_index + 2],
            };

            if (attrib.normals.size() > 0) {
                temp.nor = {
                    attrib.normals[3 * v1.normal_index + 0],
                    attrib.normals[3 * v1.normal_index + 1],
                    attrib.normals[3 * v1.normal_index + 2],
                };
            } else {
                temp.nor = manual_normal;
            }

            temp.uv = {
                attrib.texcoords[2 * v1.texcoord_index + 0],
                1.0 - attrib.texcoords[2 * v1.texcoord_index + 1], // need flip UV for vulkan
            };

            m_vertices.push_back(temp);
            // process third vertex
            temp.pos = {
                attrib.vertices[3 * v2.vertex_index + 0],
                attrib.vertices[3 * v2.vertex_index + 1],
                attrib.vertices[3 * v2.vertex_index + 2],
            };

            if (attrib.normals.size() > 0) {
                temp.nor = {
                    attrib.normals[3 * v2.normal_index + 0],
                    attrib.normals[3 * v2.normal_index + 1],
                    attrib.normals[3 * v2.normal_index + 2],
                };
            } else {
                temp.nor = manual_normal;
            }

            temp.uv = {
                attrib.texcoords[2 * v2.texcoord_index + 0],
                1.0 - attrib.texcoords[2 * v2.texcoord_index + 1], // need flip UV for vulkan
            };

            m_vertices.push_back(temp);
            // update boudung box
            box.xmin = std::min(box.xmin, attrib.vertices[3 * v0.vertex_index + 0]);
            box.xmin = std::min(box.xmin, attrib.vertices[3 * v1.vertex_index + 0]);
            box.xmin = std::min(box.xmin, attrib.vertices[3 * v2.vertex_index + 0]);

            box.ymin = std::min(box.ymin, attrib.vertices[3 * v0.vertex_index + 1]);
            box.ymin = std::min(box.ymin, attrib.vertices[3 * v1.vertex_index + 1]);
            box.ymin = std::min(box.ymin, attrib.vertices[3 * v2.vertex_index + 1]);

            box.zmin = std::min(box.zmin, attrib.vertices[3 * v0.vertex_index + 2]);
            box.zmin = std::min(box.zmin, attrib.vertices[3 * v1.vertex_index + 2]);
            box.zmin = std::min(box.zmin, attrib.vertices[3 * v2.vertex_index + 2]);

            box.xmax = std::max(box.xmax, attrib.vertices[3 * v0.vertex_index + 0]);
            box.xmax = std::max(box.xmax, attrib.vertices[3 * v1.vertex_index + 0]);
            box.xmax = std::max(box.xmax, attrib.vertices[3 * v2.vertex_index + 0]);

            box.ymax = std::max(box.ymax, attrib.vertices[3 * v0.vertex_index + 1]);
            box.ymax = std::max(box.ymax, attrib.vertices[3 * v1.vertex_index + 1]);
            box.ymax = std::max(box.ymax, attrib.vertices[3 * v2.vertex_index + 1]);

            box.zmax = std::max(box.zmax, attrib.vertices[3 * v0.vertex_index + 2]);
            box.zmax = std::max(box.zmax, attrib.vertices[3 * v1.vertex_index + 2]);
            box.zmax = std::max(box.zmax, attrib.vertices[3 * v2.vertex_index + 2]);
        }
    }
    std::cout << "box min  = " << box.xmin << ", " << box.ymin << ", " << box.zmin << std::endl;
    std::cout << "box max  = " << box.xmax << ", " << box.ymax << ", " << box.zmax << std::endl;

    /* scale based on largest extension direction  */
    double scale = (1.0 - (-1.0)) / (box.xmax - box.xmin);
    scale = std::min(scale, (1.0 - (-1.0)) / (box.ymax - box.ymin));
    scale = std::min(scale, (1.0 - (-1.0)) / (box.ymax - box.zmin));

    glm::mat4 pre_scale = glm::scale(pre_rotation, glm::vec3(scale, scale, scale));
    /* translate to centre */
    m_model_mat = glm::translate(pre_scale, glm::vec3(-0.5 * (box.xmax + box.xmin), -0.5 * (box.ymax + box.ymin), -0.5 * (box.zmax + box.zmin)));
}

glm::mat4 Mesh::get_model_mat() {
    return m_model_mat;
}