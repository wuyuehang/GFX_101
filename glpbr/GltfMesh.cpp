#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "GltfMesh.hpp"
#include "Program.hpp"
#include <iostream>

void GltfMesh::loadImages() {
    m_images.resize(gltfModel.images.size());

    for (std::vector<tinygltf::Image>::size_type i = 0; i < gltfModel.images.size(); i++) {
        tinygltf::Image & gltfImage = gltfModel.images[i];
        std::cout << "image[" << i << "].uri: " << gltfImage.uri << ", w = " << gltfImage.width
            << ", h = " << gltfImage.height << ", c = " << gltfImage.component << std::endl;

        glGenTextures(1, &m_images[i]);
        glBindTexture(GL_TEXTURE_2D, m_images[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gltfImage.width, gltfImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, gltfImage.image.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void GltfMesh::loadMaterials() {
    m_materials.resize(gltfModel.materials.size());

    for (std::vector<tinygltf::Material>::size_type i = 0; i < gltfModel.materials.size(); i++) {
        tinygltf::Material & gltfMaterial = gltfModel.materials[i];
        // get the base color factor
        if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
            m_materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
        }
        // get base color texture index
        if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
            m_materials[i].baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
        }
    }
}

void GltfMesh::loadTextures() {
    m_textures.resize(gltfModel.textures.size());

    for (std::vector<tinygltf::Texture>::size_type i = 0; i < gltfModel.textures.size(); i++) {
        m_textures[i].imageIndex = gltfModel.textures[i].source;
        std::cout << "texture[" << i << "] refers image[" << gltfModel.textures[i].source << "]" << std::endl;
    }
}

void GltfMesh::load(const std::string filename) {
    std::string warn, err;
    gltfContext.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);

    std::vector<uint32_t> indexBuffer;
    std::vector<Vertex> vertexBuffer;

    loadImages();
    loadMaterials();
    loadTextures();

    const tinygltf::Scene & scene = gltfModel.scenes[0];
    for (std::vector<int>::size_type i = 0; i < scene.nodes.size(); i++) {
        uint32_t iteration = 0;
        const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
        std::cout << "node[" << scene.nodes[i] << "].name: " << node.name << std::endl;
        loadNode(node, nullptr, indexBuffer, vertexBuffer, iteration);
    }
    // bake buffer
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertexBuffer.size(), vertexBuffer.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*indexBuffer.size(), indexBuffer.data(), GL_STATIC_DRAW);
}

void GltfMesh::loadNode(const tinygltf::Node & inputNode, Node *parent, std::vector<uint32_t> & indexBuffer, std::vector<Vertex> & vertexBuffer, uint32_t iteration) {
    Node *node = new Node {};
    node->parent = parent;
    node->matrix = glm::mat4(1.0);

    // parse local node matrix Translation-Rotation-Scale
    if (inputNode.translation.size() == 3) {
        node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
    }
    if (inputNode.rotation.size() == 4) {
        glm::quat q = glm::make_quat(inputNode.rotation.data());
        node->matrix *= glm::mat4(q);
    }
    if (inputNode.scale.size() == 3) {
        node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
    }
    if (inputNode.matrix.size() == 16) {
        node->matrix = glm::make_mat4x4(inputNode.matrix.data()); // if node.matrix is present, override TRS
    }

    // recursive tranversal children
    for (std::vector<int>::size_type i = 0; i < inputNode.children.size(); i++) {
        std::cout << std::string(2*(iteration+1), ' ') << "node.[" << inputNode.children[i] << "].name: " << gltfModel.nodes[inputNode.children[i]].name << std::endl;
        loadNode(gltfModel.nodes[inputNode.children[i]], node, indexBuffer, vertexBuffer, iteration+1);
    }

    if (inputNode.mesh > -1) {
        const tinygltf::Mesh mesh = gltfModel.meshes[inputNode.mesh];
        std::cout << std::string(2*(iteration+1), ' ') << "mesh[" << inputNode.mesh << "].name: " <<  mesh.name << std::endl;
        // iterate all primitives
        for (std::vector<tinygltf::Primitive>::size_type i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive & glTFPrimitive = mesh.primitives[i];
            uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size()); // append to a single index buffer
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size()); // append to a single vertex buffer
            uint32_t indexCount = 0;
            // Vertices
            {
                const float *positionBuffer = nullptr;
                const float *normalsBuffer = nullptr;
                const float *texCoordsBuffer = nullptr;
                size_t vertexCount = 0;

                if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor & accessor = gltfModel.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView & view = gltfModel.bufferViews[accessor.bufferView];
                    positionBuffer = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    vertexCount = accessor.count;
                } else {
                    assert(0);
                }

                if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor & accessor = gltfModel.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView & view = gltfModel.bufferViews[accessor.bufferView];
                    normalsBuffer = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                } else {
                    assert(0);
                }

                // glTF supports multiple sets, we only load the first one
                if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                    const tinygltf::Accessor & accessor = gltfModel.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView & view = gltfModel.bufferViews[accessor.bufferView];
                    texCoordsBuffer = reinterpret_cast<const float *>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                } else {
                    assert(0);
                }

                // append data to model's vertex buffer
                for (size_t v = 0; v < vertexCount; v++) {
                    Vertex vert;
                    vert.pos = glm::make_vec3(&positionBuffer[v * 3]);
                    vert.nor = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
            {
                const tinygltf::Accessor & accessor = gltfModel.accessors[glTFPrimitive.indices];
                const tinygltf::BufferView & bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer & buffer = gltfModel.buffers[bufferView.buffer];
                indexCount += static_cast<uint32_t>(accessor.count);

                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const uint32_t *buf = reinterpret_cast<const uint32_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart); // since we merge all vertex buffers into one, we need to bias the start offset of each primitive
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const uint16_t *buf = reinterpret_cast<const uint16_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const uint8_t *buf = reinterpret_cast<const uint8_t *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                default:
                    assert(0);
                }
            }
            node->mesh = new Mesh;
            node->mesh->prims.push_back(new Primitive(firstIndex, indexCount, glTFPrimitive.material));
        }
    }

    if (parent) {
        parent->children.push_back(node);
    } else {
        m_nodes.push_back(node); // global root nodes. Store in tree data structure, in GltfMesh::draw, we walk through those parent nodes.
    }
}

void GltfMesh::draw(util::Program *prog) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    for (auto node : m_nodes) {
        drawNode(node, prog);
    }
}

void GltfMesh::drawNode(Node *node, util::Program *prog) {
    if (node->mesh) {
        if (node->mesh->prims.size() > 0) {
            // traverse the node hierarchy to the top-most parent to get the final matrix of the current node
            glm::mat4 nodeMatrix = node->matrix;
            Node *currentParent = node->parent;
            while (currentParent) {
                nodeMatrix = currentParent->matrix * nodeMatrix;
                currentParent = currentParent->parent;
            }

            // update per mesh model transform
            prog->setMat4("model_mat", nodeMatrix);

            for (auto prim : node->mesh->prims) {
                if (prim->indexCount > 0) {
                    // get the texture for the primitive
                    Material & material = m_materials[prim->materialIndex];
                    Texture & texture = m_textures[material.baseColorTextureIndex];
                    GLuint image = m_images[texture.imageIndex];
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, image);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glDrawElements(GL_TRIANGLES, prim->indexCount, GL_UNSIGNED_INT, (const void *)(prim->firstIndex * sizeof(GLuint)));
                }
            }
        }
    }

    for (auto child : node->children) {
        drawNode(child, prog);
    }
}