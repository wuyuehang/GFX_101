#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Program.hpp"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl atomic counter", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);
    util::AssimpMesh mesh;

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/gltf/metal_cup_ww2_style_cup_vintage/scene.gltf");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, uv));
    glEnableVertexAttribArray(2);

    GLuint acVertice; // counter how many vertices are processed in vertex shader
    glGenBuffers(1, &acVertice);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, acVertice);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, acVertice); // binding index=0, offset=0

    GLuint acPrim; // counter how many primitives are processed in geometry shader
    glGenBuffers(1, &acPrim);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, acPrim);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, acPrim); // binding index=1, offset=0

    std::vector<std::string> shaders { "./atomic_counter.vert", "./atomic_counter.geom", "./simple.frag" };
    util::Program *P = new util::Program(shaders);
    P->use();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ctrl->handle_input();

        {
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, acVertice);
            GLuint *ptr = (GLuint *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_WRITE);
            assert(ptr);
            *ptr = 0;
            glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        }
        {
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, acPrim);
            GLuint *ptr = (GLuint *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_WRITE);
            assert(ptr);
            *ptr = 0;
            glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        }
        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        P->setMat4("model_mat", ctrl->get_model());
        P->setMat4("view_mat", ctrl->get_view());
        P->setMat4("proj_mat", ctrl->get_proj());
        assert(mesh.m_objects.size() == 1); // assume the mesh only contains 1 mesh.

        // we draw view-model transform in vertex shader and draw proj transform in geometry shader.
        // during these stages, we count how many vertices and primitives are processed in these stages.
        if (!mesh.m_objects[0].material_names.diffuse_texname.empty()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[mesh.m_objects[0].material_names.diffuse_texname]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            P->setInt("TEX0_DIFFUSE", 0);
        }
        glDrawElements(GL_TRIANGLES, mesh.m_objects[0].indexCount, GL_UNSIGNED_INT, (const void *)(mesh.m_objects[0].firstIndex * sizeof(GLuint)));

        static uint8_t log = 0;
        if (log == 0) {
            {
                glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, acVertice);
                GLuint *ptr = (GLuint *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
                assert(ptr);
                std::cout << "vertex shader visits       " << *ptr << " vertices" << std::endl;
                glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
            }
            {
                glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, acPrim);
                GLuint *ptr = (GLuint *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
                assert(ptr);
                std::cout << "geometry shader visits     " << *ptr << " primitives" << std::endl;
                glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
            }
        }
        log++;

        glfwSwapBuffers(window);
    }

    delete P;
    delete ctrl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
