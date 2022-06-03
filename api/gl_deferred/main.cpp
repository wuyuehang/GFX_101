#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Util.hpp"

util::AssimpMesh mesh;
util::Program *fwdP; util::Program *dfrP; util::Program *dfrLP;
GLuint fwdFBO, fwdTEX, fwdDEPTH;
GLuint dfrFBO, dfrPos, dfrAlbedo, dfrNor, dfrAO, dfrDEPTH;
constexpr int W = 1024;

void create_forward_pass() {
    glGenTextures(1, &fwdTEX);
    glBindTexture(GL_TEXTURE_2D, fwdTEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, W, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &fwdDEPTH);
    glBindTexture(GL_TEXTURE_2D, fwdDEPTH);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, W, W, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &fwdFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, fwdFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fwdTEX, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fwdDEPTH, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<std::string> shaders{"./forward.vert", "./forward.frag"};
    fwdP = new util::Program(shaders);
}

void create_deferred_pass() {
    // Pos_viewspace.xyzw
    glGenTextures(1, &dfrPos);
    glBindTexture(GL_TEXTURE_2D, dfrPos);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, W, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Albedo.rgba (base color or diffuse color)
    glGenTextures(1, &dfrAlbedo);
    glBindTexture(GL_TEXTURE_2D, dfrAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, W, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // N_viewspace.xyz
    glGenTextures(1, &dfrNor);
    glBindTexture(GL_TEXTURE_2D, dfrNor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, W, W, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // AO.rgb (we only illustrate)
    glGenTextures(1, &dfrAO);
    glBindTexture(GL_TEXTURE_2D, dfrAO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, W, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Depth
    glGenTextures(1, &dfrDEPTH);
    glBindTexture(GL_TEXTURE_2D, dfrDEPTH);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, W, W, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &dfrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, dfrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dfrPos, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, dfrAlbedo, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, dfrNor, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, dfrAO, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dfrDEPTH, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    {
        std::vector<std::string> shaders{"./deferred_gbuffer.vert", "./deferred_gbuffer.frag"};
        dfrP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders{"./quad.vert", "./deferred_lighting.frag"};
        dfrLP = new util::Program(shaders);
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, W, "gl deferred rendering", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

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

    create_forward_pass();
    create_deferred_pass();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ctrl->handle_input();
        glm::vec3 light_loc = glm::vec3(ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));

        // Pass 1, forward rendering
        glBindFramebuffer(GL_FRAMEBUFFER, fwdFBO);
        {
            GLenum cbuffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, cbuffers);
        }
        glViewport(0, 0, W, W);
        glEnable(GL_CULL_FACE); glFrontFace(GL_CCW); glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0); glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        fwdP->use();
        fwdP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        fwdP->setMat4("view_mat", ctrl->get_view());
        fwdP->setMat4("proj_mat", ctrl->get_proj());
        fwdP->setVec3("LightPos_viewspace", light_loc);
        for (auto & obj : mesh.m_objects) {
            assert(!obj.material_names.diffuse_texname.empty());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
            fwdP->setInt("TEX0_DIFFUSE", 0);

            assert(!obj.material_names.normal_texname.empty());
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.normal_texname]);
            fwdP->setInt("TEX3_NORMAL", 3);

            glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));
        }

        // Pass 2.1, deferred rendering gbuffer pass
        glBindFramebuffer(GL_FRAMEBUFFER, dfrFBO);
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
        dfrP->use();
        dfrP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        dfrP->setMat4("view_mat", ctrl->get_view());
        dfrP->setMat4("proj_mat", ctrl->get_proj());
        for (auto & obj : mesh.m_objects) {
            assert(!obj.material_names.diffuse_texname.empty());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
            dfrP->setInt("TEX0_ALBEDO", 0);

            assert(!obj.material_names.normal_texname.empty());
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.normal_texname]);
            dfrP->setInt("TEX3_NORMAL", 3);

            assert(!obj.material_names.ao_texname.empty());
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.ao_texname]);
            dfrP->setInt("TEX4_AO", 4);

            glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));
        }

        // Pass 2.2, deferred rendering lighting pass (fullscreen quad)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, W, W);
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(VAO);
        dfrLP->use();
        dfrLP->setVec3("LightPos_viewspace", light_loc);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dfrAlbedo);
        dfrLP->setInt("TEX0_ALBEDO", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dfrPos);
        dfrLP->setInt("TEX1_POS_VIEWSPACE", 1);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, dfrNor);
        dfrLP->setInt("TEX3_NORMAL_VIEWSPACE", 3);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
    }

    delete ctrl; delete fwdP; delete dfrP; delete dfrLP;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
