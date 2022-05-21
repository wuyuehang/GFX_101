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
#include "Program.hpp"
#include "Controller.hpp"
#include "Mesh.hpp"

util::AssimpMesh mesh; util::Program *shadowgenP;
GLuint shadowFBO, shadowTex;

util::Program *noneP; // intuitive, present shadow acne
util::Program *biasP; // add bias, improve a bit
util::Program *biaspcfP; // add bias, pcf (percentage closer filtering)

constexpr int W = 1024;
constexpr int H = 1024;
constexpr int SHADOW_W = 4096; // larger shadow map helps
constexpr int SHADOW_H = 4096;

enum SHADOW_MODE {
    NONE_MODE,
    BIAS_MODE,
    CULL_MODE,
    BIAS_PCF_MODE,
};

int shadow_mode = NONE_MODE;
util::Program *P; // point to different shadow map
float unit = 5.0;

void setup_ImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

static void create_shadowmap() {
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#if 1
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glm::vec4 borderC = glm::vec4(1.0);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &borderC[0]);
#endif
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
    glDrawBuffer(GL_NONE);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, H, "gl shadowmap", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();
    setup_ImGui(window);

    util::Controller *ctrl = new util::TrackballController(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/obj/group.obj");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    create_shadowmap();
    {
        std::vector<std::string> shaders { "./blinn-phong-shadowmap.vert", "./none.frag" };
        noneP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders { "./blinn-phong-shadowmap.vert", "./bias.frag" };
        biasP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders { "./blinn-phong-shadowmap.vert", "./bias-pcf.frag" };
        biaspcfP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders { "./shadowmap.vert", "./shadowmap.frag" };
        shadowgenP = new util::Program(shaders);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ctrl->handle_input();
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
        ImGui::Begin("Hello, OpenGL!");
        ImGui::SliderFloat("ortho size", &unit, 0.0, 20.0);
        ImGui::RadioButton("none", &shadow_mode, NONE_MODE);
        ImGui::RadioButton("bias", &shadow_mode, BIAS_MODE);
        ImGui::RadioButton("cull", &shadow_mode, CULL_MODE);
        ImGui::RadioButton("bias+pcf", &shadow_mode, BIAS_PCF_MODE);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        switch (shadow_mode) {
        case NONE_MODE:
            P = noneP;
            break;
        case BIAS_MODE:
        case CULL_MODE:
            P = biasP;
            break;
        case BIAS_PCF_MODE:
            P = biaspcfP;
            break;
        default:
            assert(0);
            break;
        }

        // Pass 1, generate shadowmap
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glViewport(0, 0, SHADOW_W, SHADOW_H);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearDepthf(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);

        glm::vec3 L_loc = glm::vec3(0.0, 0.0, 3.0);
        #if 1
        glm::mat4 lProjMat = glm::ortho<float>(-unit, unit, -unit, unit, -unit, unit);
        #else
        glm::mat4 lProjMat = glm::perspective(glm::radians(30.0), 1.0, 0.1, 100.0);
        #endif
        glm::mat4 lViewMat = glm::lookAt(L_loc, glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));

        glBindVertexArray(VAO);
        shadowgenP->use();
        shadowgenP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        shadowgenP->setMat4("view_mat", lViewMat);
        shadowgenP->setMat4("proj_mat", lProjMat);
        if (shadow_mode == CULL_MODE) {
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CCW);
            glCullFace(GL_FRONT);
        }
        mesh.draw(shadowgenP); // render the depth (distance) from light's angle.
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Pass 2, normal render
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, W, H);
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
        P->setMat4("lMVP", lProjMat * lViewMat * ctrl->get_model() * mesh.get_model_mat());
        glm::vec3 L_viewspace = glm::vec3(ctrl->get_view() * glm::vec4(L_loc, 1.0));
        P->setVec3("light_viewspace", L_viewspace);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowTex);
        P->setInt("TEX0_SHADOWMAP", 0);
        mesh.draw(P);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    delete ctrl;
    delete shadowgenP; delete noneP; delete biasP; delete biaspcfP;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
