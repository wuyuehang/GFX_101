#include "Common.hpp"
#include "Controller.hpp"
#include "Render.hpp"
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Render::Render() :m_width(1280), m_height(720), ctrl(new TrackballController(this)) {
    InitGLFW();
    InitImGui();
    CreateResource();
    BakeDefaultPipeline(VBO);
    BakeWireframePipeline(VBO);
}

Render::~Render() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
    delete ctrl;
}

void Render::CreateResource() {
    mesh.load("./assets/obj/bunny.obj", glm::mat4(1.0));
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Mesh::Vertex) * mesh.get_vertices().size(), mesh.get_vertices().data(), GL_STATIC_DRAW);

    glGenBuffers(1, &UBO);
}

void Render::BakeCommand() {
    ctrl->handle_input();

    MVP mvp_mats;
    mvp_mats.model = ctrl->get_model() * mesh.get_model_mat();
    mvp_mats.view = ctrl->get_view();
    mvp_mats.proj = ctrl->get_proj();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepthf(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (exclusive_mode == WIREFRAME_MODE) {
        GLuint prog = programs.find("WIREFRAME")->second;
        UpdateMVPUBO(UBO, prog, mvp_mats);
        glUseProgram(prog);
        GLuint vao = vaos.find("WIREFRAME")->second;
        glBindVertexArray(vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
    } else {
        GLuint prog = programs.find("DEFAULT")->second;
        UpdateMVPUBO(UBO, prog, mvp_mats);
        glUseProgram(prog);
        GLuint vao = vaos.find("DEFAULT")->second;
        glBindVertexArray(vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
    }
    glBindVertexArray(0);
}

void Render::Gameloop() {
    {
        exclusive_mode = DEFAULT_MODE;
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
        ImGui::RadioButton("Default", &exclusive_mode, DEFAULT_MODE);
        ImGui::RadioButton("Wireframe", &exclusive_mode, WIREFRAME_MODE);
        ImGui::RadioButton("Vertex Normal", &exclusive_mode, VISUALIZE_VERTEX_NORMAL_MODE);
        ImGui::RadioButton("Phong", &exclusive_mode, PHONG_MODE);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        BakeCommand();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}