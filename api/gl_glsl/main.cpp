#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <map>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Program.hpp"
#include "Util.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

constexpr int W = 1024;
std::map<std::string, util::Program *> Progs;
static int current_program_index = 0;
const char **items;
clock_t start_time;

void setup_ImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

static void create_programs() {
    std::string base_dir = std::string("./");
    std::string suffix = std::string(".frag");
    for (auto & entry : std::filesystem::directory_iterator(base_dir)) {
        std::string filename = std::string(entry.path());
        if (!std::equal(suffix.rbegin(), suffix.rend(), filename.rbegin())) {
            continue;
        }
        std::vector<std::string> shaders { "./quad.vert", filename };
        Progs.insert(std::make_pair(filename, new util::Program(shaders)));
        std::cout << "Baking : " << filename << std::endl;
    }

    items = new const char *[Progs.size()];
    uint32_t i = 0;
    for (auto & prog : Progs) {
        items[i] = prog.first.c_str();
        i++;
    }
}

int main(int argc, char *argv[]) {
    start_time = clock();
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, W, "gl glsl", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    setup_ImGui(window);

    GLuint vao;
    glGenVertexArrays(1, &vao); // need at least one vertex array object

    create_programs();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
        ImGui::Begin("Hello, OpenGL!");
        ImGui::ListBox("listbox", &current_program_index, items, Progs.size(), Progs.size());
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        clock_t current_time = clock();
        float delta = 0.00001 *(current_time - start_time);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, W, W);
        util::Program *curr_p = Progs[items[current_program_index]];
        curr_p->use();
        curr_p->setUVec2("uResInfo", W, W);
        curr_p->setFloat("uTime", delta);
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            xpos = std::max(0.0, std::min(1.0, xpos / W));
            ypos = std::max(0.0, std::min(1.0, ypos / W));
            glm::vec2 mouse = glm::vec2(2.0*float(xpos)-1.0, 2*float(ypos)-1.0);
            curr_p->setVec2("uMouse", mouse);
        }
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // cleanup
    delete [] items;
    for (auto it : Progs) {
        delete it.second;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
