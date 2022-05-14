#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Util.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main() {
    int w, h, c;
    stbi_set_flip_vertically_on_load(true);
    uint8_t *img = stbi_load("../resource/gohan.png", &w, &h, &c, STBI_rgb_alpha);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(w, h, "image load store", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::vector<std::string> shaders { "./image_load_store.comp" };
    GLuint P = gltest::CreateProgram(shaders);
    glUseProgram(P);

    // source texture
    GLuint srcObj, dstObj;
    glGenTextures(1, &srcObj);
    glBindTexture(GL_TEXTURE_2D, srcObj);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(img);

    // dst texture
    glGenTextures(1, &dstObj);
    glBindTexture(GL_TEXTURE_2D, dstObj);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindImageTexture(0, srcObj, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindImageTexture(1, dstObj, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glDispatchCompute(w, 1, 1); // change the shader local y iteration to h

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gltest::DrawTexture(dstObj);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}