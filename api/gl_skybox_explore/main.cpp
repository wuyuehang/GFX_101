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
#include "Util.hpp"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl skybox explore", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    // either we put the eye location inside the unit cube.
    util::Controller *skybox_ctrl = new util::SkyboxController(window, glm::vec3(0.0, 0.0, 3.0));
    util::AssimpMesh box;

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    box.load("../../assets/obj/sphere.obj"); // skydome
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    GLuint cubemap = gltest::CreateCubemap("../resource/skybox/red/");

    std::vector<std::string> shaders { "./skybox.vert", "./skybox.frag" };
    util::Program *P = new util::Program(shaders);
    P->use();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        skybox_ctrl->handle_input();

        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // or just scale the unit cube to contain the eye location.
        P->setMat4("model_mat", skybox_ctrl->get_model() * glm::scale(box.get_model_mat(), glm::vec3(4.0)));
        P->setMat4("view_mat", skybox_ctrl->get_view());
        P->setMat4("proj_mat", skybox_ctrl->get_proj());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
        P->setInt("Environment", 0);
        box.draw(P);
        glfwSwapBuffers(window);
    }

    delete P;
    delete skybox_ctrl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
