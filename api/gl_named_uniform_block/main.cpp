#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Mesh.hpp"
#include "Util.hpp"

GLuint VAO, UBO, P;
util::ObjMesh mesh;
glm::mat4 model_mat;
glm::mat4 view_mat = glm::lookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));
glm::mat4 proj_mat = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "uniform block", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    glViewport(0, 0, 1024, 768);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::vector<std::string> shaders { "./uniform_block.vert", "./simple.frag" };
    P = gltest::CreateProgram(shaders);
    glUseProgram(P);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/obj/torus.obj");
    model_mat = mesh.get_model_mat();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    // allocate UBO
    GLuint ubo_blk_idx = glGetUniformBlockIndex(P, "MVP_"); // uniform block name, not the instance name
    GLint ubo_sz;
    glGetActiveUniformBlockiv(P, ubo_blk_idx, GL_UNIFORM_BLOCK_DATA_SIZE, &ubo_sz);
    GLubyte *buffer = new GLubyte[ubo_sz];
    glGenBuffers(1, &UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Updata UBO
        const char *names[3] = {"MVP_.model_mat", "MVP_.view_mat", "MVP_.proj_mat"}; // uniform block name, not the instance name
        GLuint indices[3];
        GLint array_size[3];
        GLint offset[3];
        GLint type[3];

        glGetUniformIndices(P, 3, names, indices);
        glGetActiveUniformsiv(P, 3, indices, GL_UNIFORM_OFFSET, offset);
        glGetActiveUniformsiv(P, 3, indices, GL_UNIFORM_SIZE, array_size);
        glGetActiveUniformsiv(P, 3, indices, GL_UNIFORM_TYPE, type);

        std::memcpy(buffer + offset[0], &model_mat[0][0], array_size[0] * sizeof(glm::mat4(1.0)));
        std::memcpy(buffer + offset[1], &view_mat[0][0], array_size[1] * sizeof(glm::mat4(1.0)));
        std::memcpy(buffer + offset[2], &proj_mat[0][0], array_size[2] * sizeof(glm::mat4(1.0)));

        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferData(GL_UNIFORM_BUFFER, ubo_sz, buffer, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, ubo_blk_idx, UBO);

        for (auto & obj : mesh.m_objects) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
        }
        glfwSwapBuffers(window);
    }

    // free UBO
    delete [] buffer;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
