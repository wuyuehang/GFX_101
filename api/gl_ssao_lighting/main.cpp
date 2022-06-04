#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Util.hpp"

util::AssimpMesh mesh;
util::Program *gbufferP; util::Program *lightP;
GLuint gbufferFBO, gbufferPosTex, gbufferAlbedoTex, gbufferNorTex, gbufferAOTex, gbufferDEPTH;
GLuint ssaoFBO, ssaoTex;
util::Program *ssaoP; // sample with sphere
util::Program *ssaoHemiP; // sample with hemisphere
constexpr int W = 1024;
constexpr int K = 64;
std::vector<glm::vec3> ssaoSphereKernel;
std::vector<glm::vec3> ssaoHemisphereKernel; std::vector<glm::vec3> ssaoHemiNoise; GLuint ssaoNoiseTex; constexpr int NOISE_W = 4;
float radius = 0.075;
enum {
    SPHERE_AO,
    HEMISPHERE_AO,
    DEFAULT_AO,
};
int mode = SPHERE_AO;

void setup_ImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 430 core";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

// sphere kernel has disadvantage that flat face looks grey
void generate_sphere_kernel() {
    for (auto i = 0; i < K; i++) {
        glm::vec3 sample = glm::vec3(
            (2.0f * (float)rand()/RAND_MAX - 1.0f),
            (2.0f * (float)rand()/RAND_MAX - 1.0f),
            (2.0f * (float)rand()/RAND_MAX - 1.0f)
        );
        ssaoSphereKernel.push_back(sample);
    }
}

// we use TBN trick to make the random vector (sample) within hemisphere face surface normal
void generate_hemisphere_kernel() {
    for (auto i = 0; i < K; i++) {
        glm::vec3 sample = glm::vec3(
            (2.0f * (float)rand()/RAND_MAX - 1.0f), // [-1, 1]
            (2.0f * (float)rand()/RAND_MAX - 1.0f), // [-1, 1]
            ((float)rand()/RAND_MAX) // [0, 1]
        );
        ssaoHemisphereKernel.push_back(sample);
    }

    for (auto i = 0; i < NOISE_W*NOISE_W; i++) {
        // rotation around Z axis in tangent space
        glm::vec3 noise = glm::vec3(
            (2.0f * (float)rand()/RAND_MAX - 1.0f), // [-1, 1]
            (2.0f * (float)rand()/RAND_MAX - 1.0f), // [-1, 1]
            0.0
        );
        ssaoHemiNoise.push_back(noise);
    }
    glGenTextures(1, &ssaoNoiseTex);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, NOISE_W, NOISE_W, 0, GL_RGB, GL_FLOAT, glm::value_ptr(ssaoHemiNoise[0]));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // we will upscale the UV upon texturing.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void create_ssao_pass() {
    // ssao.r
    glGenTextures(1, &ssaoTex);
    glBindTexture(GL_TEXTURE_2D, ssaoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, W, W, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    {
        std::vector<std::string> shaders{"./quad.vert", "./ssao.frag"};
        ssaoP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders{"./quad.vert", "./ssao_hemi.frag"};
        ssaoHemiP = new util::Program(shaders);
    }
}

void create_deferred_pass() {
    // Pos_viewspace.xyzw
    glGenTextures(1, &gbufferPosTex);
    glBindTexture(GL_TEXTURE_2D, gbufferPosTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, W, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Albedo.rgba (base color or diffuse color)
    glGenTextures(1, &gbufferAlbedoTex);
    glBindTexture(GL_TEXTURE_2D, gbufferAlbedoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, W, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // N_viewspace.xyz
    glGenTextures(1, &gbufferNorTex);
    glBindTexture(GL_TEXTURE_2D, gbufferNorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, W, W, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // AO.rgb
    glGenTextures(1, &gbufferAOTex);
    glBindTexture(GL_TEXTURE_2D, gbufferAOTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, W, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Depth
    glGenTextures(1, &gbufferDEPTH);
    glBindTexture(GL_TEXTURE_2D, gbufferDEPTH);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, W, W, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &gbufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbufferPosTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gbufferAlbedoTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gbufferNorTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gbufferAOTex, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferDEPTH, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    {
        std::vector<std::string> shaders{"./deferred_gbuffer.vert", "./deferred_gbuffer.frag"};
        gbufferP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders{"./quad.vert", "./deferred_lighting.frag"};
        lightP = new util::Program(shaders);
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, W, "gl ssao lighting", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();
    setup_ImGui(window);

    util::Controller *ctrl = new util::TrackballController(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/obj/bag.obj");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, tan));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, bta));
    glEnableVertexAttribArray(4);
    glBindVertexArray(0);

    generate_sphere_kernel();
    generate_hemisphere_kernel();
    create_ssao_pass();
    create_deferred_pass();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ctrl->handle_input();
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // Pin the UI
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
        ImGui::Begin(" ");
        ImGui::SliderFloat("radius", &radius, 0.0, 0.5);
        ImGui::RadioButton("AO sphere", &mode, SPHERE_AO);
        ImGui::RadioButton("AO hemisphere", &mode, HEMISPHERE_AO);
        ImGui::RadioButton("AO default", &mode, DEFAULT_AO);
        ImGui::End();
        ImGui::Render();
        glm::vec3 light_loc = glm::vec3(ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));

        // Pass 1.1, deferred rendering gbuffer pass
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
        {
            GLenum cbuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
            glDrawBuffers(4, cbuffers);
        }
        glViewport(0, 0, W, W);
        glEnable(GL_CULL_FACE); glFrontFace(GL_CCW); glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
        GLfloat zero[] = { 0.0, 0.0, 0.0, 0.0 };
        glClearBufferfv(GL_COLOR, 0, zero); glClearBufferfv(GL_COLOR, 1, zero);
        glClearBufferfv(GL_COLOR, 2, zero); glClearBufferfv(GL_COLOR, 3, zero);
        glClearDepthf(1.0); glClear(GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        gbufferP->use();
        gbufferP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        gbufferP->setMat4("view_mat", ctrl->get_view());
        gbufferP->setMat4("proj_mat", ctrl->get_proj());
        for (auto & obj : mesh.m_objects) {
            assert(!obj.material_names.diffuse_texname.empty());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
            gbufferP->setInt("TEX0_ALBEDO", 0);

            assert(!obj.material_names.normal_texname.empty());
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.normal_texname]);
            gbufferP->setInt("TEX3_NORMAL", 3);

            assert(!obj.material_names.ao_texname.empty());
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.ao_texname]);
            gbufferP->setInt("TEX4_AO", 4);

            glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));
        }

        if (mode == SPHERE_AO) {
            // Pass 1.2 ssao pass (sphere)
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
            glViewport(0, 0, W, W);
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(VAO);
            ssaoP->use();
            ssaoP->setFloat("radius", radius);
            {
                GLuint _p = ssaoP->program();
                glUniform3fv(glGetUniformLocation(_p, "ssaoKernel"), K, glm::value_ptr(ssaoSphereKernel[0]));
            }
            ssaoP->setMat4("proj_mat", ctrl->get_proj());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gbufferPosTex);
            ssaoP->setInt("TEX1_POS_VIEWSPACE", 1);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        } else if (mode == HEMISPHERE_AO) {
            // Pass 1.2 ssao pass (hemisphere)
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
            glViewport(0, 0, W, W);
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(VAO);
            ssaoHemiP->use();
            ssaoHemiP->setFloat("radius", radius);
            {
                GLuint _p = ssaoHemiP->program();
                glUniform3fv(glGetUniformLocation(_p, "ssaoKernel"), K, glm::value_ptr(ssaoHemisphereKernel[0]));
            }
            ssaoHemiP->setMat4("proj_mat", ctrl->get_proj());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gbufferPosTex);
            ssaoHemiP->setInt("TEX1_POS_VIEWSPACE", 1);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, gbufferNorTex);
            ssaoHemiP->setInt("TEX3_NORMAL_VIEWSPACE", 3);

            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, ssaoNoiseTex);
            ssaoHemiP->setInt("TEX6_SSAO_NOISE", 6);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Pass 1.2, deferred rendering lighting pass (fullscreen quad)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, W, W);
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(VAO);
        lightP->use();
        lightP->setVec3("LightPos_viewspace", light_loc);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferAlbedoTex);
        lightP->setInt("TEX0_ALBEDO", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferPosTex);
        lightP->setInt("TEX1_POS_VIEWSPACE", 1);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gbufferNorTex);
        lightP->setInt("TEX3_NORMAL_VIEWSPACE", 3);

        glActiveTexture(GL_TEXTURE4);
        if (mode == DEFAULT_AO) {
            glBindTexture(GL_TEXTURE_2D, gbufferAOTex);
        } else {
            glBindTexture(GL_TEXTURE_2D, ssaoTex);
        }
        lightP->setInt("TEX4_AO", 4);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    delete ctrl; delete gbufferP; delete lightP; delete ssaoP; delete ssaoHemiP;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
