#include "Common.hpp"
#include "Controller.hpp"
#include "PBRRender.hpp"
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void PBRRender::InitGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(m_width, m_height, "OpenGL PBR", nullptr, nullptr);
    glfwMakeContextCurrent(m_window);

    glewInit(); // let GLEW handles function pointers so that all GL commands can be called then.
}

void PBRRender::InitImGui() {
    // setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

PBRRender::PBRRender(int argc, char *argv[]) :m_width(1280), m_height(720) {
    InitGLFW();
    m_ctrl = new TrackballController(m_window);
    InitImGui();
    if (argc == 2) {
        mesh.load(std::string(argv[1]));
    } else {
        mesh.load("../assets/gltf/old_spot_mini_rigged/scene.gltf");
    }
    BakeVAO();
    BakeDefaultPipeline();
    BakeWireframePipeline();
    BakeDiffuseSpecularPipeline();
    BakePhongPipeline();
}

PBRRender::~PBRRender() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (auto it : progs) {
        delete it.second;
    }

    glfwDestroyWindow(m_window);
    glfwTerminate();
    delete m_ctrl;
}

void PBRRender::BakeCommand() {
    m_ctrl->handle_input();
    run_if_default();
    run_if_wireframe();
    run_if_diffuse_specular();
    run_if_phong();
}

void PBRRender::Gameloop() {
    {
        m_exclusive_mode = DEFAULT_MODE;
        m_roughness = 1.0;
    }
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // describe the ImGui UI
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Always);
        ImGui::Begin("Hello, OpenGL!");
        ImGui::RadioButton("Default", &m_exclusive_mode, DEFAULT_MODE);
        ImGui::RadioButton("Phong", &m_exclusive_mode, PHONG_MODE);
        ImGui::RadioButton("Wireframe", &m_exclusive_mode, WIREFRAME_MODE);
        ImGui::RadioButton("Diffuse & Specular", &m_exclusive_mode, DIFFUSE_SPECULAR_MODE);
        ImGui::SliderFloat("Roughness", &m_roughness, 0.01, 256.0);
        ImGui::Text("Eye @ (%.1f, %.1f, %.1f)", m_ctrl->get_eye().x, m_ctrl->get_eye().y, m_ctrl->get_eye().z);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        BakeCommand();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}

int main(int argc, char *argv[]) {
    PBRRender app(argc, argv);
    app.Gameloop();
    return 0;
}
