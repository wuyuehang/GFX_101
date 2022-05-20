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
#include "Util.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int w, h, c; // make window size adaptive
GLuint srcTex;
GLuint grayscaleFBO, grayscaleTex; util::Program *grayscaleP; float threshold = 0.5;
GLuint vblur_FBO, vblur_Tex; util::Program *vblurP;
GLuint hblur_FBO, hblur_Tex; util::Program *hblurP; int blur_loop = 1;
GLuint mergeFBO, mergeTex; util::Program *mergeP; float bloom_degree = 2.0;

void setup_ImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

constexpr int W = 5; // gaussian blur radius size, include (0, 0) centre
GLfloat pWeights[W] {};

static float GaussianWeight(float offset) {
    constexpr float pi = 3.1415926;
    constexpr float sigma = 3.0;
    return 1.0/sigma/sqrt(2.0*pi)*exp(-0.5*offset*offset/sigma/sigma);
}

static void init_gaussian_filter() {
    float sum = 0;
    for (auto i = 0; i < W; i++) {
        pWeights[i] = GaussianWeight(i); // GaussianWeight(W, i / W);
        sum += pWeights[i];
        std::cout << "pWeight[" << i << "] = " << pWeights[i] << std::endl;
    }
    sum *= 1.0;
    std::cout << "sum(pWeights) " << sum << std::endl;
    for (auto i = 0; i < W; i++) {
        pWeights[i] /= sum;
        std::cout << "pWeight[" << i << "] = " << pWeights[i] << std::endl;
    }
}

static void create_render_target(GLuint & FBO, GLuint & TEX) {
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glGenTextures(1, &TEX);
    glBindTexture(GL_TEXTURE_2D, TEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TEX, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_brightness_pass() {
    create_render_target(grayscaleFBO, grayscaleTex);
    std::vector<std::string> shaders { "quad.vert", "grayscale.frag" };
    grayscaleP = new util::Program(shaders);
}

void create_gaussian_v_pass() {
    create_render_target(vblur_FBO, vblur_Tex);
    std::vector<std::string> shaders { "quad.vert", "vblur.frag" };
    vblurP = new util::Program(shaders);
}

void create_gaussian_h_pass() {
    create_render_target(hblur_FBO, hblur_Tex);
    std::vector<std::string> shaders { "quad.vert", "hblur.frag" };
    hblurP = new util::Program(shaders);
}

void create_merge_pass() {
    create_render_target(mergeFBO, mergeTex);
    std::vector<std::string> shaders { "quad.vert", "merge.frag" };
    mergeP = new util::Program(shaders);
}

int main(int argc, char *argv[]) {
    uint8_t *img;
    if (argc == 2) {
        img = stbi_load(argv[1], &w, &h, &c, STBI_rgb_alpha);
    } else {
        img = stbi_load("../resource/bloom.jpg", &w, &h, &c, STBI_rgb_alpha);
    }
    assert(img);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(w, h, "gl bloom", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    setup_ImGui(window);

    glGenTextures(1, &srcTex);
    glBindTexture(GL_TEXTURE_2D, srcTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(img);

    init_gaussian_filter();
    create_brightness_pass();
    create_gaussian_h_pass();
    create_gaussian_v_pass();
    create_merge_pass();

    GLuint vao;
    glGenVertexArrays(1, &vao); // need at least one vertex array object

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(300, 130), ImGuiCond_Always);
        ImGui::Begin("Hello, OpenGL!");
        ImGui::SliderFloat("b threshold", &threshold, 0.0, 1.0);
        ImGui::SliderInt("blur loop", &blur_loop, 0, 8);
        ImGui::SliderFloat("bloom degree", &bloom_degree, 1.0, 10.0);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        // Pass 1, calculate brightness
        glBindFramebuffer(GL_FRAMEBUFFER, grayscaleFBO);
        glViewport(0, 0, w, h);
        grayscaleP->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcTex);
        grayscaleP->setInt("TEX0_DIFFUSE", 0);
        grayscaleP->setFloat("threshold", threshold);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Pass 1.1, blit horizon blur's render target with previous brightness result
        glBindFramebuffer(GL_READ_FRAMEBUFFER, grayscaleFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hblur_FBO);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Pass 2, gaussian blur
        for (auto i = 0; i < blur_loop; i++) {
            {
                glBindFramebuffer(GL_FRAMEBUFFER, vblur_FBO);
                glViewport(0, 0, w, h);
                vblurP->use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, hblur_Tex);
                vblurP->setInt("TEX0_DIFFUSE", 0);
                vblurP->setFloatArray("uGaussianWeights", W, pWeights);
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            {
                glBindFramebuffer(GL_FRAMEBUFFER, hblur_FBO);
                glViewport(0, 0, w, h);
                hblurP->use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, vblur_Tex);
                hblurP->setInt("TEX0_DIFFUSE", 0);
                hblurP->setFloatArray("uGaussianWeights", W, pWeights);
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // Pass 3, merge
        glBindFramebuffer(GL_FRAMEBUFFER, mergeFBO);
        glViewport(0, 0, w, h);
        mergeP->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcTex);
        mergeP->setInt("TEX0_ORIGIN", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, hblur_Tex);
        mergeP->setInt("TEX1_BLUR", 1);
        mergeP->setFloat("bloom_degree", bloom_degree);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Pass 4, final result
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, w, h);
        gltest::DrawTexture(mergeTex);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    delete hblurP;
    delete vblurP;
    delete grayscaleP;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
