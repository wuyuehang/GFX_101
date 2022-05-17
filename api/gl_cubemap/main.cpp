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
#include "stb_image.h"

util::AssimpMesh box;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl cubemap", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    box.load("../../assets/obj/cube.obj");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    std::string prefix = std::string("../resource/skybox/sun/");
    std::vector<std::string> env = {
        "right.png", "left.png", "top.png",
        "bottom.png", "back.png", "front.png"
    };
    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    int w, h, c;
    uint8_t *ptr;
    for (auto i = 0; i < 6; i++) {
        std::string fullname = prefix + env[i];
        ptr = stbi_load(fullname.c_str(), &w, &h, &c, STBI_rgb_alpha);
        assert(ptr);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
        stbi_image_free(ptr);
        ptr = nullptr;
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::vector<std::string> shaders { "./simple.vert", "./cubemap.frag" };
    util::Program *P = new util::Program(shaders);
    P->use();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ctrl->handle_input();

        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        P->setMat4("model_mat", ctrl->get_model() * glm::scale(box.get_model_mat(), glm::vec3(0.5)));
        P->setMat4("view_mat", ctrl->get_view());
        P->setMat4("proj_mat", ctrl->get_proj());
        assert(box.m_objects[0].material_names.diffuse_texname.empty());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
        P->setInt("Environment", 0);
        box.draw(P);
        glfwSwapBuffers(window);
    }

    delete P;
    delete ctrl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
