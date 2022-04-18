#include <cassert>

#include "tiny_obj_loader.h"
#include "Mesh.hpp"

void Mesh::load(const std::string filename) {
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

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex temp {};

            temp.pos = {
                attrib.vertices[3 * index.vertex_index],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };

            temp.nor = {
                attrib.normals[3 * index.normal_index],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2],
            };

            temp.uv = {
                attrib.texcoords[2 * index.texcoord_index],
                1.0 - attrib.texcoords[2 * index.texcoord_index + 1], // need flip UV for vulkan
            };

            m_vertices.push_back(temp);
        }
    }
}