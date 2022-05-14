#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Program.hpp"
#include "Util.hpp"

util::AssimpMesh mesh;

GLuint VAO;
util::Program *P;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl instance", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);
    mesh.load("../../assets/gltf/metal_cup_ww2_style_cup_vintage/scene.gltf");

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    std::vector<std::string> shaders { "./instance_uniform.vert", "./simple.frag" };
    P = new util::Program(shaders);

    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0), glm::vec3(0.25));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ctrl->handle_input();

        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto i = 0; i < 4; i++) {
            for (auto j = 0; j < 4; j++) {
                glm::vec3 offset;
                offset.x = (GLfloat)i * 2 / 4 - 1.0;
                offset.y = (GLfloat)j * 2 / 4 - 1.0;
                offset.z = 0.0;
                glm::mat4 translate_mat = glm::translate(glm::mat4(1.0), offset);
                std::string name = "model_mat[" + std::to_string(i*4+j) + "]";
                P->setMat4(name, translate_mat * ctrl->get_model() * scale_mat);
            }
        }
        P->setMat4("view_mat", ctrl->get_view());
        P->setMat4("proj_mat", ctrl->get_proj());
        P->use();
        glBindVertexArray(VAO);
        for (auto obj : mesh.m_objects) {
            if (!obj.material_names.diffuse_texname.empty()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                P->setInt("TEX0_DIFFUSE", 0);
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.indexbuf_id);
            glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
            glDrawElementsInstanced(GL_TRIANGLES, obj.indices.size(), GL_UNSIGNED_INT, 0, 16);
        }
        glfwSwapBuffers(window);
    }

    delete P;
    delete ctrl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
