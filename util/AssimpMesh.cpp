#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Mesh.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace util {

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
                std::cout << "diffuse_texture: " << texture_filename << ", w = " << w << ", h = "
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
                std::cout << "specular_texture: " << texture_filename << ", w = " << w << ", h = "
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
                std::cout << "roughness_texture: " << texture_filename << ", w = " << w << ", h = "
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
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_NORMALS, 0, &path)) {
            if (m_textures.find(path.data) == m_textures.end()) {
                std::string texture_filename = base_dir + path.data;
                int w, h, c;
                uint8_t *ptr = stbi_load(texture_filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
                assert(ptr);
                std::cout << "normal_texture: " << texture_filename << ", w = " << ", h = "
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
        if (AI_SUCCESS == aiGetMaterialTexture(pMaterial, aiTextureType_NORMALS, 0, &path)) {
            o.material_names.normal_texname = path.data;
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
