#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <vector>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "stb_image.h"
#include "Common.hpp"
#include "Program.hpp"
#include "Util.hpp"

constexpr int W = 1024;
constexpr int H = 1024;
static float CameraZ = 8.0; // camera world position
static float LightZ = 1.0; // light world position
util::Program *pointlightP;
util::Program *spotlightP;
util::Program *P;
bool is_spot = false;

void setup_ImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, H, "gl spotlight vs pointlight", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();
    setup_ImGui(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    GLuint VBO, EBO;
    std::vector<uint32_t> idx { 0, 1, 2, 0, 2, 3 };
    std::vector<Vertex> pos {
        { glm::vec3(-1.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 0.0) },
        { glm::vec3(-1.0,  1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(0.0, 1.0) },
        { glm::vec3( 1.0,  1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 1.0) },
        { glm::vec3( 1.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 0.0) },
    };
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*pos.size(), pos.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    {
        std::vector<std::string> shaders{ "./simple.vert", "./spotlight.frag" };
        spotlightP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders{ "./simple.vert", "./pointlight.frag" };
        pointlightP = new util::Program(shaders);
    }

    int w, h, c;
    uint8_t *img = stbi_load("../resource/floor.jpg", &w, &h, &c, STBI_rgb_alpha);
    GLuint TEX;
    glGenTextures(1, &TEX);
    glBindTexture(GL_TEXTURE_2D, TEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(img);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin(" ");
        ImGui::Checkbox("Spotlight", &is_spot);
        ImGui::SliderFloat("Camera.Z", &CameraZ, 0.0, 20.0);
        ImGui::SliderFloat("Light.Z", &LightZ, 0.0, 2.0);
        ImGui::End();
        ImGui::Render();

        P = (is_spot) ? spotlightP : pointlightP;

        glm::mat4 view_mat = glm::lookAt(glm::vec3(0.0, 0.0, CameraZ), glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 proj_mat = glm::perspective(glm::radians(45.0f), (float)W/H, 0.01f, 50.0f);
        glm::vec3 LightPos_viewspace = glm::vec3(view_mat * glm::vec4(0.0, 0.0, LightZ, 1.0));

        glViewport(0, 0, W, H);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        P->use();
        P->setMat4("model_mat", glm::scale(glm::mat4(1.0), glm::vec3(4.0)));
        P->setMat4("view_mat", view_mat);
        P->setMat4("proj_mat", proj_mat);
        P->setVec3("LightPos_viewspace", LightPos_viewspace);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TEX);
        P->setInt("TEX0_DIFFUSE", 0);

        if (is_spot) {
            P->setFloat("SpotlightCutoff", 0.5); // 30x2 degree cone
            glm::vec3 SpotlightAttenuation = glm::vec3(0.0, 0.0, -1.0); // negative Z
            P->setVec3("SpotlightAttenuation", SpotlightAttenuation);
        }
        glDrawElements(GL_TRIANGLES, idx.size(), GL_UNSIGNED_INT, (const void *)0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    delete spotlightP; delete pointlightP;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}