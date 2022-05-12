#ifndef GLTFMESH_HPP
#define GLTFMESH_HPP

#include "Mesh.hpp"
#include "Program.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

// gltf concept
// scene --> node --> mesh --> primitive --> accessor --> buffer_view --> buffer
//                      !
//                      !--> material --> texture --> image
//                                           !
//                                           !--> sampler
class GltfMesh : public util::MeshBase {
private:
    // material.texture
    struct Texture {
        uint32_t imageIndex;
    };
    // mesh.material
    struct Material {
        glm::vec4 baseColorFactor = glm::vec4(1.0);
        uint32_t baseColorTextureIndex;
    };
    // mesh.primitives
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t materialIndex;
        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t materialIndex) : firstIndex(firstIndex), indexCount(indexCount), materialIndex(materialIndex) {}
    };
    // node.mesh
    struct Mesh {
        std::vector<Primitive *> prims;
        ~Mesh() {
            for (auto p : prims) {
                delete p;
            }
        }
    };
    // scene.node
    struct Node {
        Node *parent;
        std::vector<Node *> children;
        Mesh *mesh;
        glm::mat4 matrix;
        ~Node() {
            if (mesh) {
                delete mesh;
            }
            for (auto child : children) {
                delete child;
            }
        }
    };
public:
    GltfMesh(const GltfMesh &) = delete;
    GltfMesh() {};
    ~GltfMesh() {};
    void load(const std::string) override final;
    void draw(util::Program *);
    std::vector<Node *> m_nodes;
private:
    void loadImages();
    void loadMaterials();
    void loadTextures();
    void loadNode(const tinygltf::Node & inputNode, Node *parent, std::vector<uint32_t> & indexBuffer, std::vector<Vertex> & vertexBuffer, uint32_t iteration);
    void drawNode(Node *, util::Program *);
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfContext;
    GLuint VBO;
    GLuint EBO;
    std::vector<GLuint> m_images;
    std::vector<Texture> m_textures;
    std::vector<Material> m_materials;
};
#endif