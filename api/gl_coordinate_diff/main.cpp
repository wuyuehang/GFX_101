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

util::AssimpMesh mesh; util::Program *P;
GLuint gbufferFBO, PosViewspaceTex, PosProjspaceTex, PosDivisionTex, PosBiasTex, gbufferDepth; util::Program *gbufferP;
GLuint diffFBO, diffTex; util::Program *diffP;

constexpr int W = 1024;
constexpr int H = 1024;

static void create_diff_depth() {
    glGenFramebuffers(1, &diffFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, diffFBO);
    glGenTextures(1, &diffTex);
    glBindTexture(GL_TEXTURE_2D, diffTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, W, H, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, diffTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void create_gbuffer() {
    glGenFramebuffers(1, &gbufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
    // Position_Viewspace.xyzw (SV_Target0)
    glGenTextures(1, &PosViewspaceTex);
    glBindTexture(GL_TEXTURE_2D, PosViewspaceTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, PosViewspaceTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));

    // Position_Projspace.xyzw (SV_Target1)
    glGenTextures(1, &PosProjspaceTex);
    glBindTexture(GL_TEXTURE_2D, PosProjspaceTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, PosProjspaceTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));

    // Manually perform Position_Projspace.xyzw/Position_Projspace.w (SV_Target2)
    glGenTextures(1, &PosDivisionTex);
    glBindTexture(GL_TEXTURE_2D, PosDivisionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, PosDivisionTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));

    // Manually perform Position_Projspace.xyzw/Position_Projspace.w, remap from [-1, 1] to [0, 1] (SV_Target3)
    glGenTextures(1, &PosBiasTex);
    glBindTexture(GL_TEXTURE_2D, PosBiasTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, PosBiasTex, 0);
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
    GLFWwindow *window = glfwCreateWindow(W, H, "gl coordinate space", nullptr, nullptr);
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

    create_diff_depth();
    create_gbuffer();
    {
        std::vector<std::string> shaders { "./quad.vert", "./sub.frag" };
        diffP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders { "./quad.vert", "./default.frag" };
        P = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders { "./gbuffer.vert", "./gbuffer.frag" };
        gbufferP = new util::Program(shaders);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ctrl->handle_input();

        // Pass 1, Position_Viewspace/Perspectivespace in gbuffer pass
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
        {
            GLenum cbuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
            glDrawBuffers(4, cbuffers);
        }
        glViewport(0, 0, W, H);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearDepthf(1.0);
        GLfloat infinity[] = { 100.0, 100.0, 100.0, 1.0 }; // clear to special value represent +infinity far
        glClearBufferfv(GL_COLOR, 0, infinity);
        glClearBufferfv(GL_COLOR, 1, infinity);
        glClearBufferfv(GL_COLOR, 2, infinity);
        glClearBufferfv(GL_COLOR, 3, infinity);
        glClear(GL_DEPTH_BUFFER_BIT);// | GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        gbufferP->use();
        gbufferP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        gbufferP->setMat4("view_mat", ctrl->get_view());
        gbufferP->setMat4("proj_mat", ctrl->get_proj());
        mesh.draw(gbufferP);

        // Pass 2, compare in reference (HW) generate depth against manual calculated depth. Need to inspect the texture in renderdoc
        glBindFramebuffer(GL_FRAMEBUFFER, diffFBO);
        {
            GLenum cbuffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, cbuffers);
        }
        glViewport(0, 0, W, H);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        diffP->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferDepth);
        diffP->setInt("TEX0_DEPTH_REF", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, PosBiasTex); // our manual calculated depth
        diffP->setInt("TEX1_DEPTH_MANUAL", 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Pass 3, default renderpass gives a rough result
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, W, H);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearDepthf(1.0);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        P->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferDepth);
        P->setInt("TEX0_DEPTH_REF", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, PosBiasTex); // our manual calculated depth
        P->setInt("TEX1_DEPTH_MANUAL", 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
    }

    delete ctrl;
    delete gbufferP; delete P; delete diffP;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
