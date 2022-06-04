#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Controller.hpp"
#include "Mesh.hpp"

constexpr int W = 1024;
util::AssimpMesh mesh;
util::Program *xP, *nmP, *hmP;
GLuint nmFBO, nmTex; // normalmap
GLuint hmFBO, hmTex; // heighmap + normalmap
GLuint xFBO, xTex; // no heightmap or normalmap

void create_pass(GLuint & TEX, GLuint & FB) {
    glGenTextures(1, &TEX);
    glBindTexture(GL_TEXTURE_2D, TEX);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, W, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint RB;
    glGenRenderbuffers(1, &RB);
    glBindRenderbuffer(GL_RENDERBUFFER, RB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, W);

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
    nmP = new util::Program(shaders);
    create_pass(nmTex, nmFBO);
}

// normal comes from geometry mesh
void setup_default_pass() {
    std::vector<std::string> shaders{"./simple.vert", "./simple.frag"};
    xP = new util::Program(shaders);
    create_pass(xTex, xFBO);
}

void setup_height_map_pass() {
    std::vector<std::string> shaders{"./height.vert", "./height.frag"};
    hmP = new util::Program(shaders);
    create_pass(hmTex, hmFBO);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(W, W, "gl heightmap", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/obj/stone-floor-medieval/terra_entrada.obj");
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
    setup_height_map_pass();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ctrl->handle_input();
        glm::vec3 light_loc = glm::vec3(ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));
        GLenum cbuffers[] = { GL_COLOR_ATTACHMENT0 };

        // Pass 1, normalmap pass
        glBindFramebuffer(GL_FRAMEBUFFER, nmFBO);
        glDrawBuffers(1, cbuffers);
        glViewport(0, 0, W, W);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        nmP->use();
        nmP->setMat4("model_mat", ctrl->get_model());
        nmP->setMat4("view_mat", ctrl->get_view());
        nmP->setMat4("proj_mat", ctrl->get_proj());
        nmP->setVec3("LightPos_viewspace", light_loc);

        auto & obj = mesh.m_objects[0];
        assert(!obj.material_names.diffuse_texname.empty());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
        nmP->setInt("TEX0_DIFFUSE", 0);
        assert(!obj.material_names.normal_texname.empty());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.normal_texname]);
        nmP->setInt("TEX3_NORMAL", 3);
        glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));

        // Pass 2, default pass
        glBindFramebuffer(GL_FRAMEBUFFER, xFBO);
        glDrawBuffers(1, cbuffers);
        glViewport(0, 0, W, W);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        xP->use();
        xP->setMat4("model_mat", ctrl->get_model());
        xP->setMat4("view_mat", ctrl->get_view());
        xP->setMat4("proj_mat", ctrl->get_proj());
        xP->setVec3("LightPos_viewspace", light_loc);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
        xP->setInt("TEX0_DIFFUSE", 0);
        glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));

        // Pass 3, heightmap pass
        glBindFramebuffer(GL_FRAMEBUFFER, hmFBO);
        glDrawBuffers(1, cbuffers);
        glViewport(0, 0, W, W);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(VAO);
        hmP->use();
        hmP->setMat4("model_mat", ctrl->get_model());
        hmP->setMat4("view_mat", ctrl->get_view());
        hmP->setMat4("proj_mat", ctrl->get_proj());
        hmP->setVec3("LightPos_viewspace", light_loc);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.diffuse_texname]);
        hmP->setInt("TEX0_DIFFUSE", 0);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.normal_texname]);
        hmP->setInt("TEX3_NORMAL", 3);
        assert(!obj.material_names.displacement_texname.empty());
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, mesh.m_textures[obj.material_names.displacement_texname]);
        hmP->setInt("TEX7_HEIGHT", 7);
        glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));

        // Pass 4
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, W, W);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, nmFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, W/3, W, 0, 0, W/3, W, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, hmFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(W/3, 0, W*2/3, W, W/3, 0, W*2/3, W, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, xFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(W*2/3, 0, W, W, W*2/3, 0, W, W, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glfwSwapBuffers(window);
    }

    delete ctrl;
    delete xP; delete nmP; delete hmP;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}