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
    float vertices[] = {
        -0.5, -0.5, 0.0, 1.0, 0.0, 0.0,
        0.5, -0.5, 0.0, 0.0, 1.0, 0.0,
        0.0, 0.5, 0.0, 0.0, 0.0, 1.0,
    };

    GLuint vbo, vao;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const void*)12);
    glEnableVertexAttribArray(1);

    VS = BuildShaderProgram("./shaders/simple.vert", GL_VERTEX_SHADER);
    GLuint FS = BuildShaderProgram("./shaders/simple.frag", GL_FRAGMENT_SHADER);
    GLuint prog = BuildProgramPipeline();

    glGenBuffers(1, &UBO);

    glUseProgramStages(prog, GL_VERTEX_SHADER_BIT, VS);
    glUseProgramStages(prog, GL_FRAGMENT_SHADER_BIT, FS);
}

void Render::BakeCommand() {
    ctrl->handle_input();
    {
        MVP mvp_mats;
        mvp_mats.model = ctrl->get_model();
        mvp_mats.view = ctrl->get_view();
        mvp_mats.proj = ctrl->get_proj();

        UpdateMVPUBO(UBO, VS, mvp_mats);
    }
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepthf(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Render::Gameloop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // describe the ImGui UI
        ImGui::Begin("Hello, OpenGL!");
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Always);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        BakeCommand();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}