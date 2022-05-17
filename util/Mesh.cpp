#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Mesh.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace util {

void ObjMesh::draw(util::Program *prog) {
    for (auto obj : m_objects) {
        if (obj.material_id > 0) {
            std::string diffuse_tex = m_materials[obj.material_id].diffuse_texname;
            if (m_textures.find(diffuse_tex) != m_textures.end()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_textures[diffuse_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setInt("TEX0_DIFFUSE", 0);
                prog->setVec3("material.Kd", obj.material.Kd);
            }

            std::string specular_tex = m_materials[obj.material_id].specular_texname;
            if (m_textures.find(specular_tex) != m_textures.end()) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, m_textures[specular_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setInt("TEX1_SPECULAR", 1);
                prog->setVec3("material.Ks", obj.material.Ks);
            }

            std::string roughness_tex = m_materials[obj.material_id].roughness_texname;
            if (m_textures.find(roughness_tex) != m_textures.end()) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, m_textures[roughness_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setInt("TEX2_ROUGHNESS", 2);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
    }
}

void ObjMesh::load(const std::string filename) {
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
    assert(filename.find_last_of("/\\") != std::string::npos);
    std::string base_dir = filename.substr(0, filename.find_last_of("/\\"));
    base_dir += "/";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &m_materials, &warn, &err, filename.c_str(), base_dir.c_str())) {
        assert(0);
    }

    std::cout << "vertice  = " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "normal   = " << attrib.normals.size() / 3 << std::endl;
    std::cout << "uv       = " << attrib.texcoords.size() / 2 << std::endl;
    std::cout << "material = " << m_materials.size() << std::endl;
    std::cout << "shape    = " << shapes.size() << std::endl;

    for (std::vector<tinyobj::material_t>::size_type i = 0; i < m_materials.size(); i++) {
        std::cout << "material[" << i << "].diffuse_texname = " << m_materials[i].diffuse_texname << std::endl;
        std::cout << "material[" << i << "].specular_texname = " << m_materials[i].specular_texname << std::endl;
        std::cout << "material[" << i << "].roughness_texname = " << m_materials[i].roughness_texname << std::endl;
    }

    for (std::vector<tinyobj::material_t>::size_type i = 0; i < m_materials.size(); i++) {
        if (m_materials[i].diffuse_texname.length() > 0) {
            if (m_textures.find(m_materials[i].diffuse_texname) == m_textures.end()) {
                std::string texture_filename = base_dir + m_materials[i].diffuse_texname; // assume in the same directory
                int w, h, c;
                uint8_t *ptr = stbi_load(texture_filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
                assert(ptr);
                std::cout << "diffuse_texture: " << texture_filename << ", w = "<< w << ", h = "
                    << h << ", comp = " << c << std::endl;

                GLuint tid;
                glGenTextures(1, &tid);
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
                glBindTexture(GL_TEXTURE_2D, 0);
                stbi_image_free(ptr);

                m_textures.insert(std::make_pair(m_materials[i].diffuse_texname, tid));
            }
        }
        if (m_materials[i].specular_texname.length() > 0) {
            if (m_textures.find(m_materials[i].specular_texname) == m_textures.end()) {
                std::string texture_filename = base_dir + m_materials[i].specular_texname; // assume in the same directory
                int w, h, c;
                uint8_t *ptr = stbi_load(texture_filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
                assert(ptr);
                std::cout << "specular_texture: " << texture_filename << ", w = "<< w << ", h = "
                    << h << ", comp = " << c << std::endl;

                GLuint tid;
                glGenTextures(1, &tid);
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
                glBindTexture(GL_TEXTURE_2D, 0);
                stbi_image_free(ptr);

                m_textures.insert(std::make_pair(m_materials[i].specular_texname, tid));
            }
        }
        if (m_materials[i].roughness_texname.length() > 0) {
            if (m_textures.find(m_materials[i].roughness_texname) == m_textures.end()) {
                std::string texture_filename = base_dir + m_materials[i].roughness_texname; // assume in the same directory
                int w, h, c;
                uint8_t *ptr = stbi_load(texture_filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
                assert(ptr);
                std::cout << "roughness_texture: " << texture_filename << ", w = "<< w << ", h = "
                    << h << ", comp = " << c << std::endl;

                GLuint tid;
                glGenTextures(1, &tid);
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
                glBindTexture(GL_TEXTURE_2D, 0);
                stbi_image_free(ptr);

                m_textures.insert(std::make_pair(m_materials[i].roughness_texname, tid));
            }
        }
    }

    box.xmax = box.ymax = box.zmax = -std::numeric_limits<float>::max();
    box.xmin = box.ymin = box.zmin = std::numeric_limits<float>::max();

    for (const auto& shape : shapes) {
        DrawObj o {}; // For simple, assume it is per shape material (TODO: support per face material)
        // Faces are defined using lists of vertex, texture and normal indices in the format vertex_index/texture_index/normal_index
        // f 3/3/1 2/2/1 1/1/1
        // f 1/1/1 4/4/1 3/3/1
        // assume a face is a triangle
        for (std::vector<tinyobj::index_t>::size_type f = 0; f < shape.mesh.indices.size() / 3; f++) {
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

            if (attrib.texcoords.size() > 0) {
                temp.uv = {
                    attrib.texcoords[2 * v0.texcoord_index + 0],
                    1.0 - attrib.texcoords[2 * v0.texcoord_index + 1], // need flip UV for vulkan
                };
            } else {
                temp.uv = glm::vec2(0.0);
            }

            o.vertices.push_back(temp);
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

            if (attrib.texcoords.size() > 0) {
                temp.uv = {
                    attrib.texcoords[2 * v1.texcoord_index + 0],
                    1.0 - attrib.texcoords[2 * v1.texcoord_index + 1], // need flip UV for vulkan
                };
            } else {
                temp.uv = glm::vec2(0.0);
            }

            o.vertices.push_back(temp);
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

            if (attrib.texcoords.size() > 0) {
                temp.uv = {
                    attrib.texcoords[2 * v2.texcoord_index + 0],
                    1.0 - attrib.texcoords[2 * v2.texcoord_index + 1], // need flip UV for vulkan
                };
            } else {
                temp.uv = glm::vec2(0.0);
            }

            o.vertices.push_back(temp);
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
        // BAKE VERTEX
        assert(o.vertices.size() > 0);
        glGenBuffers(1, &o.buffer_id);
        glBindBuffer(GL_ARRAY_BUFFER, o.buffer_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*o.vertices.size(), o.vertices.data(), GL_STATIC_DRAW);
        // LOAD DIFFUSE & SPECULAR
        if (shape.mesh.material_ids.size() > 0) {
            o.material_id = shape.mesh.material_ids[0]; // use the material ID of first face
        } else {
            o.material_id = -1;
        }
        // MATERIAL REFLEXITY
        if (o.material_id != -1) {
            o.material.Ka = glm::vec3(m_materials[o.material_id].ambient[0], m_materials[o.material_id].ambient[1], m_materials[o.material_id].ambient[2]);
            o.material.Kd = glm::vec3(m_materials[o.material_id].diffuse[0], m_materials[o.material_id].diffuse[1], m_materials[o.material_id].diffuse[2]);
            o.material.Ks = glm::vec3(m_materials[o.material_id].specular[0], m_materials[o.material_id].specular[1], m_materials[o.material_id].specular[2]);
        }
        m_objects.push_back(o);
    }
    std::cout << "box min  = " << box.xmin << ", " << box.ymin << ", " << box.zmin << std::endl;
    std::cout << "box max  = " << box.xmax << ", " << box.ymax << ", " << box.zmax << std::endl;

    /* scale based on largest extension direction  */
    double scale = 0.5 * (box.xmax - box.xmin);
    scale = std::max(scale, 0.5 * (box.ymax - box.ymin));
    scale = std::max(scale, 0.5 * (box.zmax - box.zmin));
    scale = 1.0 / scale;
    /* translate to centre */
    m_model_mat = glm::translate(glm::mat4(1.0), glm::vec3(-0.5 * (box.xmax + box.xmin), -0.5 * (box.ymax + box.ymin), -0.5 * (box.zmax + box.zmin)));
    m_model_mat = glm::scale(glm::mat4(1.0), glm::vec3(scale, scale, scale)) * m_model_mat;
}

void AssimpMesh::load(const std::string filename) {
    assert(filename.find_last_of("/\\") != std::string::npos);
    std::string base_dir = filename.substr(0, filename.find_last_of("/\\"));
    base_dir += "/";
    Assimp::Importer Importer;
    // aiProcess_OptimizeMeshes
    auto pScene = Importer.ReadFile(filename.c_str(), aiProcess_Triangulate |
        aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
        aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords | aiProcess_FlipUVs);
    assert(pScene);

    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        const aiMaterial *pMaterial = pScene->mMaterials[i];

        aiString path;
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_DIFFUSE, 0, &path)) {
            if (m_textures.find(path.data) == m_textures.end()) {
                std::string texture_filename = base_dir + path.data;
                int w, h, c;
                uint8_t *ptr = stbi_load(texture_filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
                assert(ptr);
                std::cout << "diffuse_texture: " << texture_filename << ", w = "<< w << ", h = "
                    << h << ", comp = " << c << std::endl;

                GLuint tid;
                glGenTextures(1, &tid);
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
                glBindTexture(GL_TEXTURE_2D, 0);
                stbi_image_free(ptr);

                m_textures.insert(std::make_pair(path.data, tid));
            }
        }
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_SPECULAR, 0, &path)) {
            if (m_textures.find(path.data) == m_textures.end()) {
                std::string texture_filename = base_dir + path.data;
                int w, h, c;
                uint8_t *ptr = stbi_load(texture_filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
                assert(ptr);
                std::cout << "specular_texture: " << texture_filename << ", w = "<< w << ", h = "
                    << h << ", comp = " << c << std::endl;

                GLuint tid;
                glGenTextures(1, &tid);
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
                glBindTexture(GL_TEXTURE_2D, 0);
                stbi_image_free(ptr);

                m_textures.insert(std::make_pair(path.data, tid));
            }
        }
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_DIFFUSE_ROUGHNESS, 0, &path)) {
            if (m_textures.find(path.data) == m_textures.end()) {
                std::string texture_filename = base_dir + path.data;
                int w, h, c;
                uint8_t *ptr = stbi_load(texture_filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
                assert(ptr);
                std::cout << "roughness_texture: " << texture_filename << ", w = "<< w << ", h = "
                    << h << ", comp = " << c << std::endl;

                GLuint tid;
                glGenTextures(1, &tid);
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
                glBindTexture(GL_TEXTURE_2D, 0);
                stbi_image_free(ptr);

                m_textures.insert(std::make_pair(path.data, tid));
            }
        }
    }

    box.xmax = box.ymax = box.zmax = -std::numeric_limits<float>::max();
    box.xmin = box.ymin = box.zmin = std::numeric_limits<float>::max();
    for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
        DrawObj o {};
        const aiMesh *pMesh = pScene->mMeshes[i];

        for (unsigned int n = 0; n < pMesh->mNumVertices; n++) {
            const aiVector3D *pos = &(pMesh->mVertices[n]);
            const aiVector3D *nor = &(pMesh->mNormals[n]);
            const aiVector3D *uv = &(pMesh->mTextureCoords[0][n]);
            o.vertices.push_back(Vertex { glm::vec3(pos->x, pos->y, pos->z), glm::vec3(nor->x, nor->y, nor->z), glm::vec2(uv->x, uv->y) });

            // update boudung box
            box.xmin = std::min(box.xmin, pos->x);
            box.ymin = std::min(box.ymin, pos->y);
            box.zmin = std::min(box.zmin, pos->z);

            box.xmax = std::max(box.xmax, pos->x);
            box.ymax = std::max(box.ymax, pos->y);
            box.zmax = std::max(box.zmax, pos->z);
        }

        for (unsigned int n = 0; n < pMesh->mNumFaces; n++) {
            const aiFace & face = pMesh->mFaces[n];
            assert(face.mNumIndices == 3);
            o.indices.push_back(face.mIndices[0]);
            o.indices.push_back(face.mIndices[1]);
            o.indices.push_back(face.mIndices[2]);
        }
        std::cout << "vertice  = " << pMesh->mNumVertices << std::endl;
        // BAKE VERTEX && INDICE
        assert(o.vertices.size() > 0);
        glGenBuffers(1, &o.buffer_id);
        glBindBuffer(GL_ARRAY_BUFFER, o.buffer_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*o.vertices.size(), o.vertices.data(), GL_STATIC_DRAW);
        glGenBuffers(1, &o.indexbuf_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, o.indexbuf_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*o.indices.size(), o.indices.data(), GL_STATIC_DRAW);
        // BAKE MATERIAL
        const aiMaterial *pMaterial = pScene->mMaterials[pMesh->mMaterialIndex];
        aiString path;
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_DIFFUSE, 0, &path)) {
            o.material_names.diffuse_texname = path.data;
        }
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_SPECULAR, 0, &path)) {
            o.material_names.specular_texname = path.data;
        }
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_DIFFUSE_ROUGHNESS, 0, &path)) {
            o.material_names.roughness_texname = path.data;
        }
        aiColor4D K;
        if (AI_SUCCESS == aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_AMBIENT, &K)) {
            o.material.Ka = glm::vec3(K.r, K.g, K.b);
        }
        if (AI_SUCCESS == aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_DIFFUSE, &K)) {
            o.material.Kd = glm::vec3(K.r, K.g, K.b);
        }
        if (AI_SUCCESS == aiGetMaterialColor(pMaterial, AI_MATKEY_COLOR_SPECULAR, &K)) {
            o.material.Ks = glm::vec3(K.r, K.g, K.b);
        }
        m_objects.push_back(o);
    }
    std::cout << "material = " << pScene->mNumMaterials << std::endl;
    std::cout << "shape    = " << pScene->mNumMeshes << std::endl;

    std::cout << "box min  = " << box.xmin << ", " << box.ymin << ", " << box.zmin << std::endl;
    std::cout << "box max  = " << box.xmax << ", " << box.ymax << ", " << box.zmax << std::endl;

    /* scale based on largest extension direction  */
    double scale = 0.5 * (box.xmax - box.xmin);
    scale = std::max(scale, 0.5 * (box.ymax - box.ymin));
    scale = std::max(scale, 0.5 * (box.zmax - box.zmin));
    scale = 1.0 / scale;
    /* translate to centre */
    m_model_mat = glm::translate(glm::mat4(1.0), glm::vec3(-0.5 * (box.xmax + box.xmin), -0.5 * (box.ymax + box.ymin), -0.5 * (box.zmax + box.zmin)));
    m_model_mat = glm::scale(glm::mat4(1.0), glm::vec3(scale, scale, scale)) * m_model_mat;
}

void AssimpMesh::draw(util::Program *prog) {
    for (auto obj : m_objects) {
        if (!obj.material_names.diffuse_texname.empty()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_textures[obj.material_names.diffuse_texname]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            prog->setInt("TEX0_DIFFUSE", 0);
            prog->setVec3("material.Kd", obj.material.Kd);
        }

        if (!obj.material_names.specular_texname.empty()) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_textures[obj.material_names.specular_texname]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            prog->setInt("TEX1_SPECULAR", 1);
            prog->setVec3("material.Ks", obj.material.Ks);
        }

        if (!obj.material_names.roughness_texname.empty()) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_textures[obj.material_names.roughness_texname]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            prog->setInt("TEX2_ROUGHNESS", 2);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.indexbuf_id);
        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawElements(GL_TRIANGLES, obj.indices.size(), GL_UNSIGNED_INT, 0);
    }
}
}
