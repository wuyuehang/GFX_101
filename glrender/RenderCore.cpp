#include "Render.hpp"
#include <iostream>

void _dbgCB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *userParam) {
    std::cout <<"(" << id << "): " << msg << std::endl;
}

void Render::InitGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(m_width, m_height, "HelloOpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(m_window);

    glewInit(); // let GLEW handles function pointers so that all GL commands can be called then.

#if GLRENDER_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(_dbgCB, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif
}

void Render::InitImGui() {
    // setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}