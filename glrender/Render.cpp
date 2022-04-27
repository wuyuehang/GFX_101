#include "Common.hpp"
#include "Controller.hpp"
#include "Render.hpp"
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <SOIL/SOIL.h>

Render::Render() :m_width(1280), m_height(720), ctrl(new TrackballController(this)) {
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
    delete ctrl;
}

void Render::CreateResource() {
    glm::mat4 pre_rotation_mat = glm::rotate(glm::mat4(1.0), (float)glm::radians(-90.0), glm::vec3(1.0, 0.0, 0.0));
    pre_rotation_mat = glm::rotate(glm::mat4(1.0), (float)glm::radians(-90.0), glm::vec3(0.0, 1.0, 0.0)) * pre_rotation_mat;
    mesh.load("./assets/obj/Buddha.obj", pre_rotation_mat);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Mesh::Vertex) * mesh.get_vertices().size(), mesh.get_vertices().data(), GL_STATIC_DRAW);

    glGenBuffers(1, &UBO);

    int w, h, c;
    uint8_t *ptr = SOIL_load_image("./assets/obj/Buddha.jpg", &w, &h, &c, SOIL_LOAD_RGBA);
    assert(ptr);
    glGenTextures(1, &TEX);
    glBindTexture(GL_TEXTURE_2D, TEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
    SOIL_free_image_data(ptr);
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
        Program *pg = progs.find("WIREFRAME")->second;
        pg->updateMVPUBO(UBO, mvp_mats);
        pg->use();
        GLuint vao = vaos.find("WIREFRAME")->second;
        glBindVertexArray(vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
    } else if (exclusive_mode == PHONG_MODE) {
        Program *pg = progs.find("PHONG")->second;
        pg->updateMVPUBO(UBO, mvp_mats);
        glm::vec3 light_world_loc = glm::vec3(0.0, 0.0, 3.0);
        light_world_loc = glm::mat3(ctrl->get_view()) * light_world_loc;
        pg->setVec3("view_loc", light_world_loc);
        pg->setFloat("shiness", shiness);
        pg->use();
        GLuint vao = vaos.find("PHONG")->second;
        glBindVertexArray(vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
    } else if (exclusive_mode == VISUALIZE_VERTEX_NORMAL_MODE) {
        Program *pg = progs.find("VISUALIZE_VERTEX_NORMAL")->second;
        pg->updateMVPUBO(UBO, mvp_mats);
        pg->use();
        GLuint vao = vaos.find("VISUALIZE_VERTEX_NORMAL")->second;
        glBindVertexArray(vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
    } else {
        Program *pg = progs.find("DEFAULT")->second;
        pg->updateMVPUBO(UBO, mvp_mats);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TEX);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        pg->setInt("TEX0", 0);
        pg->use();
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
        shiness = 1.0;
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
        ImGui::SliderFloat("Shiness", &shiness, 1.0, 256.0);
        ImGui::Text("Eye @ (%.1f, %.1f, %.1f)", ctrl->get_eye().x, ctrl->get_eye().y, ctrl->get_eye().z);
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();

        BakeCommand();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}