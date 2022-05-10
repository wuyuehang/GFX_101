#ifndef GLTFMESH_HPP
#define GLTFMESH_HPP

#include "Mesh.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

class Program;

class GltfMesh : public MeshBase {
private:
    // mesh.primitives
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        Primitive(uint32_t firstIndex, uint32_t indexCount) : firstIndex(firstIndex), indexCount(indexCount) {}
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
    void draw(Program *);
    std::vector<Node *> m_nodes;
private:
    void loadNode(const tinygltf::Node & inputNode, Node *parent, std::vector<uint32_t> & indexBuffer, std::vector<Vertex> & vertexBuffer, uint32_t iteration);
    void drawNode(Node *, Program *);
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfContext;
    GLuint VBO;
    GLuint EBO;
};
#endif