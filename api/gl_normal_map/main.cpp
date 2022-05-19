#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Util.hpp"

util::AssimpMesh mesh;
util::Program *P, *normalP;
GLuint ltex, lrb, lfb;
GLuint rtex, rrb, rfb;

void create_pass(GLuint & TEX, GLuint & RB, GLuint & FB) {
    glGenTextures(1, &TEX);
    glBindTexture(GL_TEXTURE_2D, TEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1024, 768, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenRenderbuffers(1, &RB);
    glBindRenderbuffer(GL_RENDERBUFFER, RB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 768);

    glGenFramebuffers(1, &FB);
    glBindFramebuffer(GL_FRAMEBUFFER, FB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TEX, 0);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RB);
    assert(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// normal comes from normal map
void setup_normal_map_pass() {
    std::vector<std::string> shaders{"./normal.vert", "./normal.frag"};
    normalP = new util::Program(shaders);
    create_pass(ltex, lrb, lfb);
}

// normal comes from geometry mesh
void setup_default_pass() {
    std::vector<std::string> shaders{"./simple.vert", "./simple.frag"};
    P = new util::Program(shaders);
    create_pass(rtex, rrb, rfb);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl normal map", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/gltf/game_boy_classic/scene.gltf");
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

    setup_default_pass();
    setup_normal_map_pass();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ctrl->handle_input();
        glm::vec3 light_loc = glm::vec3(ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));
        GLenum cbuffers[] = { GL_COLOR_ATTACHMENT0 };

        // Pass 1
        glBindFramebuffer(GL_FRAMEBUFFER, lfb);
        glDrawBuffers(1, cbuffers);
        glViewport(0, 0, 1024, 768);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        glBindVertexArray(VAO);
        normalP->use();
        normalP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        normalP->setMat4("view_mat", ctrl->get_view());
        normalP->setMat4("proj_mat", ctrl->get_proj());
        normalP->setVec3("LightPos_viewspace", light_loc);

        auto & obj = mesh.m_objects[0];
        assert(!obj.material_names.diffuse_texname.empty());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        normalP->setInt("TEX0_DIFFUSE", 0);
        assert(!obj.material_names.normal_texname.empty());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.normal_texname]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        normalP->setInt("TEX3_NORMAL", 3);
        glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));

        // Pass 2
        glBindFramebuffer(GL_FRAMEBUFFER, rfb);
        glDrawBuffers(1, cbuffers);
        glViewport(0, 0, 1024, 768);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        glBindVertexArray(VAO);
        P->use();
        P->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat());
        P->setMat4("view_mat", ctrl->get_view());
        P->setMat4("proj_mat", ctrl->get_proj());
        P->setVec3("LightPos_viewspace", light_loc);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        P->setInt("TEX0_DIFFUSE", 0);
        glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));

        // Pass 3
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, 1024, 768);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, lfb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, 1024/2, 768, 0, 0, 1024/2, 768, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, rfb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(1024/2, 0, 1024, 768, 1024/2, 0, 1024, 768, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glfwSwapBuffers(window);
    }

    delete ctrl;
    delete P;
    delete normalP;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}