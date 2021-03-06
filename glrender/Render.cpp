#include "Common.hpp"
#include "Controller.hpp"
#include "Render.hpp"
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Render::Render(int argc, char *argv[]) :m_width(1280), m_height(720) {
    InitGLFW();
    m_ctrl = new util::TrackballController(m_window);
    InitImGui();
    if (argc == 2) {
        mesh.load(std::string(argv[1]));
    } else {
        mesh.load("../assets/gltf/old_spot_mini_rigged/scene.gltf");
    }
    BakeVAO();
    BakeDefaultPipeline();
    BakeWireframePipeline();
    BakeVVNPipeline();
    BakeDiffuseSpecularPipeline();
    BakePhongPipeline();
    BakeToonPipeline();
    BakeVisualizeZViewportPipeline(); // raw gl_FragCoord.z
    BakeVisualizeZViewspacePipeline(); // inverse transfrom from viewport to view space.
}

Render::~Render() {
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

void Render::BakeCommand() {
    m_ctrl->handle_input();
    run_if_default();
    run_if_wireframe();
    run_if_vvn();
    run_if_diffuse_specular();
    run_if_phong();
    run_if_toon();
    run_if_visualize_z_in_viewport();
    run_if_visualize_z_in_viewspace();
}

void Render::Gameloop() {
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
        ImGui::SetNextWindowSize(ImVec2(250, 250), ImGuiCond_Always);
        ImGui::Begin("Hello, OpenGL!");
        ImGui::RadioButton("Default", &m_exclusive_mode, DEFAULT_MODE);
        ImGui::RadioButton("Phong", &m_exclusive_mode, PHONG_MODE);
        ImGui::RadioButton("Wireframe", &m_exclusive_mode, WIREFRAME_MODE);
        ImGui::RadioButton("Vertex Normal", &m_exclusive_mode, VISUALIZE_VERTEX_NORMAL_MODE);
        ImGui::RadioButton("Toon", &m_exclusive_mode, TOON_MODE);
        ImGui::RadioButton("Z depth (viewport space)", &m_exclusive_mode, VISUALIZE_Z_IN_VIEWPORT_MODE);
        ImGui::RadioButton("Z depth (view space)", &m_exclusive_mode, VISUALIZE_Z_IN_VIEWSPACE_MODE);
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
    Render app(argc, argv);
    app.Gameloop();
    return 0;
}
