#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

constexpr int W = 600;
constexpr int H = 600;

void setup_ImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsClassic();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, H, "calc", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();
    setup_ImGui(window);

    glm::vec4 input = glm::vec4(0.0, 0.0, 0.0, 1.0);
    glm::vec4 output;
    // view transform
    glm::vec3 eye = glm::vec3(0.0, 0.0, 3.0);
    glm::vec3 at = glm::vec3(0.0);
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    // perspective transform
    float fov = 30.0;
    float ratio = 1.0;
    float p_near = 0.1;
    float p_far = 100.0;
    // orthographic
    float o_left = -5.0; float o_right = 5.0;
    float o_bot = -5.0; float o_top = 5.0;
    float o_near = -5.0; float o_far = 5.0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_None); // Pin the UI
        ImGui::Begin(" ");
        ImGui::Text("World Space");
        ImGui::InputFloat4("input", &input[0]);
        // view transform
        ImGui::InputFloat3("view_mat.eye", &eye[0]);
        ImGui::InputFloat3("view_mat.at", &at[0]);
        ImGui::InputFloat3("view_mat.up", &up[0]);
        glm::mat4 ViewMat = glm::lookAt(eye, at, up);
        output = ViewMat * input;
        ImGui::Text("View (Camera) Space");
        ImGui::InputFloat4("output", &output[0]);
        // perspective transform
        ImGui::InputFloat("pers_mat.fov", &fov);
        ImGui::InputFloat("pers_mat.ratio", &ratio);
        ImGui::InputFloat("pers_mat.near", &p_near);
        ImGui::InputFloat("pers_mat.far", &p_far);
        glm::mat4 PersMat = glm::perspective(glm::radians(fov), ratio, p_near, p_far);
        output = PersMat * ViewMat * input;
        ImGui::Text("Perspective Projection Clip Space");
        ImGui::InputFloat4("output", &output[0]);
        // orthographic transform
        float f32_one = 1.0;
        ImGui::InputScalar("orth_mat.left", ImGuiDataType_Float, &o_left, &f32_one);
        ImGui::InputScalar("orth_mat.right", ImGuiDataType_Float, &o_right, &f32_one);
        ImGui::InputScalar("orth_mat.bottom", ImGuiDataType_Float, &o_bot, &f32_one);
        ImGui::InputScalar("orth_mat.top", ImGuiDataType_Float, &o_top, &f32_one);
        ImGui::InputScalar("orth_mat.near", ImGuiDataType_Float, &o_near, &f32_one);
        ImGui::InputScalar("orth_mat.far", ImGuiDataType_Float, &o_far, &f32_one);
        glm::mat4 OrthMat = glm::ortho<float>(o_left, o_right, o_bot, o_top, o_near, o_far);
        output = OrthMat * ViewMat * input;
        ImGui::Text("Orthographic Projection Clip Space");
        ImGui::InputFloat4("output", &output[0]);
        ImGui::End();
        ImGui::Render();

        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
