#ifndef MESH_HPP
#define MESH_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Common.hpp"
#include "tiny_obj_loader.h"

class Mesh {
public:
    virtual ~Mesh() = 0;
    virtual void load(const std::string) = 0;
    glm::mat4 get_model_mat() const { return m_model_mat; }
protected:
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

class ObjMesh : public Mesh {
public:
    struct DrawObj {
        GLuint buffer_id;
        size_t material_id;
        std::vector<Vertex> vertices; // can be pruned
        Material material;
    };
    ObjMesh(const ObjMesh &) = delete;
    ObjMesh() {};
    ~ObjMesh() override {};
    void load(const std::string) override final;
    void bind_diffuse(DrawObj &, uint32_t);
    void bind_specular(DrawObj &, uint32_t);
    void bind_roughness(DrawObj &, uint32_t);
    void draw(DrawObj &);
    std::vector<DrawObj> m_objects;
    std::vector<tinyobj::material_t> m_materials;
    std::map<std::string, GLuint> m_textures;
};

class AssimpMesh : public Mesh {
public:
    struct material_t {
        std::string diffuse_texname;
        std::string specular_texname;
    };
    struct DrawObj {
        GLuint buffer_id;
        GLuint indexbuf_id;
        std::vector<Vertex> vertices; // can be pruned
        std::vector<uint32_t> indices; // can be pruned
        material_t material_names;
        Material material;
    };
    AssimpMesh(const AssimpMesh &) = delete;
    AssimpMesh() {};
    ~AssimpMesh() override {};
    void load(const std::string) override final;
    void bind_diffuse(DrawObj &, uint32_t);
    void bind_specular(DrawObj &, uint32_t);
    void bind_roughness(DrawObj &, uint32_t);
    void draw(DrawObj &);
    std::vector<DrawObj> m_objects;
    std::map<std::string, GLuint> m_textures;
};
#endif
