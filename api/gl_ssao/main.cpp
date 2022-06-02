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

#define enable_dbg 0
util::AssimpMesh mesh;
GLuint gbufferFBO; GLuint gbufferTEX, gbufferDepth; util::Program *gbufferP;
util::Program *dbgP;
constexpr int W = 1024;
constexpr int H = 1024;
constexpr int K = 64;
glm::vec3 grandom[K];
GLuint dbgFBO; GLuint dbgTEX;

static void generate_kernel() {
    for (auto i = 0; i < K; i++) {
        float scale = 0.01;
        grandom[i].x = 2.0f * (float)rand()/RAND_MAX - 1.0f; // [-1, 1]
        grandom[i].y = 2.0f * (float)rand()/RAND_MAX - 1.0f;
        grandom[i].z = 2.0f * (float)rand()/RAND_MAX - 1.0f;
        // scale
        grandom[i].x = grandom[i].x * scale;
        grandom[i].y = grandom[i].y * scale;
        grandom[i].z = grandom[i].z * scale;
    }
}
#if enable_dbg
static void create_dbg_renderpass() {
    glGenFramebuffers(1, &dbgFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, dbgFBO);
    glGenTextures(1, &dbgTEX);
    glBindTexture(GL_TEXTURE_2D, dbgTEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dbgTEX, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif

static void create_gbuffer() {
    glGenFramebuffers(1, &gbufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
    // Position_Viewspace.xyzw
    glGenTextures(1, &gbufferTEX);
    glBindTexture(GL_TEXTURE_2D, gbufferTEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbufferTEX, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glGenTextures(1, &gbufferDepth);
    glBindTexture(GL_TEXTURE_2D, gbufferDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, W, H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferDepth, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, H, "gl ssao", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/obj/group.obj");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    create_gbuffer();
#if enable_dbg
    create_dbg_renderpass();
#endif
    generate_kernel();
    {
        std::vector<std::string> shaders { "./gbuffer.vert", "./gbuffer.frag" };
        gbufferP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders { "./quad.vert", "./dbg.frag" };
        dbgP = new util::Program(shaders);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ctrl->handle_input();

        // Pass 1, gbuffer pass, generate position in viewspace
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
        {
            GLenum cbuffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, cbuffers);
        }
        glViewport(0, 0, W, H);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearDepthf(1.0);
        GLfloat infinity[] = { 100.0, 100.0, 100.0, 1.0 }; // clear to special value represent +infinity far
        glClearBufferfv(GL_COLOR, 0, infinity);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        gbufferP->use();
        gbufferP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        gbufferP->setMat4("view_mat", ctrl->get_view());
        gbufferP->setMat4("proj_mat", ctrl->get_proj());
        mesh.draw(gbufferP);

        // Pass 2, debug pass
#if enable_dbg
        glBindFramebuffer(GL_FRAMEBUFFER, dbgFBO);
        glViewport(0, 0, W, H);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        dbgP->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTEX);
        dbgP->setInt("TEX0_POS_VIEWSPACE", 0);
        dbgP->setMat4("proj_mat", ctrl->get_proj());
        {
            GLuint _p = dbgP->program();
            glUniform3fv(glGetUniformLocation(_p, "grandom"), K, glm::value_ptr(grandom[0]));
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);
#endif

        // Pass 3
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, W, H);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearDepthf(1.0);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        dbgP->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTEX);
        dbgP->setInt("TEX0_POS_VIEWSPACE", 0);
        dbgP->setMat4("proj_mat", ctrl->get_proj());
        {
            GLuint _p = dbgP->program();
            glUniform3fv(glGetUniformLocation(_p, "grandom"), K, glm::value_ptr(grandom[0]));
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
    }

    delete ctrl;
    delete gbufferP; delete dbgP;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
