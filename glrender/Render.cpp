#include "Common.hpp"
#include "Controller.hpp"
#include "Render.hpp"
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <SOIL/SOIL.h>

Render::Render() :m_width(1280), m_height(720), m_ctrl(new TrackballController(this)) {
    InitGLFW();
    InitImGui();
    CreateResource();
    BakeDefaultPipeline(VBO);
    BakeWireframePipeline(VBO);
    BakeVVNPipeline(VBO);
    BakePhongPipeline(VBO);
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

void Render::CreateResource() {
    mesh.load("./assets/obj/Buddha.obj", glm::mat4(1.0));
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.get_vertices().size(), mesh.get_vertices().data(), GL_STATIC_DRAW);

    int w, h, c;
    uint8_t *ptr = SOIL_load_image("./assets/obj/Buddha.jpg", &w, &h, &c, SOIL_LOAD_RGBA);
    assert(ptr);
    glGenTextures(1, &TEX);
    glBindTexture(GL_TEXTURE_2D, TEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
    SOIL_free_image_data(ptr);
}

void Render::BakeCommand() {
    m_ctrl->handle_input();
    run_if_default(VBO);
    run_if_wireframe(VBO);
    run_if_vvn(VBO);
    run_if_phong(VBO);
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
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Always);
        ImGui::Begin("Hello, OpenGL!");
        ImGui::RadioButton("Default", &m_exclusive_mode, DEFAULT_MODE);
        ImGui::RadioButton("Wireframe", &m_exclusive_mode, WIREFRAME_MODE);
        ImGui::RadioButton("Vertex Normal", &m_exclusive_mode, VISUALIZE_VERTEX_NORMAL_MODE);
        ImGui::RadioButton("Phong", &m_exclusive_mode, PHONG_MODE);
        ImGui::SliderFloat("Shiness", &m_roughness, 0.01, 256.0);
        ImGui::Text("Eye @ (%.1f, %.1f, %.1f)", m_ctrl->get_eye().x, m_ctrl->get_eye().y, m_ctrl->get_eye().z);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        BakeCommand();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}

int main() {
    Render app;
    app.Gameloop();
    return 0;
}