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

util::AssimpMesh mesh;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl clip distance", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/gltf/metal_cup_ww2_style_cup_vintage/scene.gltf");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    std::vector<std::string> shaders { "./simple.vert", "./simple.frag" };
    util::Program *P = new util::Program(shaders);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ctrl->handle_input();

        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);
        P->use();
        P->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        P->setMat4("view_mat", ctrl->get_view());
        P->setMat4("proj_mat", ctrl->get_proj());
        glEnable(GL_CLIP_DISTANCE0);
        mesh.draw(P);

        glfwSwapBuffers(window);
    }

    delete ctrl;
    delete P;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}