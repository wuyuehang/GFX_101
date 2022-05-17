#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Program.hpp"
#include "Util.hpp"

GLuint meshVAO;
util::AssimpMesh mesh;
util::Program *meshP;
GLfloat Outer_level = 1.0;
GLfloat Inner_level = 1.0;

void setup_ImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void setup_mesh() {
    glGenVertexArrays(1, &meshVAO);
    glBindVertexArray(meshVAO);

    mesh.load("../../assets/obj/cube.obj"); // buffers are bound now.

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    std::vector<std::string> shaders{"./simple.vert", "./simple.tesc", "./simple.tese", "./simple.frag"};
    meshP = new util::Program(shaders);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl tessellation explore", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    setup_ImGui(window);

    util::Controller *ctrl = new util::TrackballController(window, glm::vec3(0.0, 0.0, 3.0));
    setup_mesh();

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    // for simplicity, we treat original triangle as control patch, and tessellate it (the triangle domain)
    // to generate smaller triangles.
    GLint MaxPatchVertices = 0;
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
    std::cout << "GL_MAX_PATCH_VERTICES: " << MaxPatchVertices << std::endl;
    glPatchParameteri(GL_PATCH_VERTICES, 3);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ctrl->handle_input();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin("Hello, OpenGL!");
        ImGui::SliderFloat("Outer level", &Outer_level, 1.0, 10.0);
        ImGui::SliderFloat("Inner level", &Inner_level, 1.0, 10.0);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(meshVAO);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        meshP->use();
        meshP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat() * glm::scale(glm::mat4(1.0), glm::vec3(0.5)));
        meshP->setMat4("view_mat", ctrl->get_view());
        meshP->setMat4("proj_mat", ctrl->get_proj());
        meshP->setFloat("uOuterLevel", Outer_level);
        meshP->setFloat("uInnerLevel", Inner_level);
        for (auto obj : mesh.m_objects) {
            glDrawElements(GL_PATCHES, obj.indices.size(), GL_UNSIGNED_INT, 0);
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    delete meshP;
    delete ctrl;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}