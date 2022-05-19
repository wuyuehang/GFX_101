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
#include "Program.hpp"
#include "Util.hpp"

GLuint skyboxVAO;
util::AssimpMesh skybox;
util::Program *skyboxP;
GLuint skyboxTEX;

GLuint meshVAO;
util::AssimpMesh mesh;
util::Program *meshP;

util::Program *reflectP;
util::Program *refractP;

void setup_mesh() {
    glGenVertexArrays(1, &meshVAO);
    glBindVertexArray(meshVAO);

    mesh.load("../../assets/obj/Buddha.obj"); // buffers are bound now.

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    
    std::vector<std::string> shaders{"./simple.vert", "./simple.frag"};
    meshP = new util::Program(shaders);
}

void setup_skybox() {
    glGenVertexArrays(1, &skyboxVAO);
    glBindVertexArray(skyboxVAO);

    skybox.load("../../assets/obj/cube.obj"); // buffers are bound now.

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void *)offsetof(AdvVertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    skyboxTEX = gltest::CreateCubemap("../resource/skybox/space/");
    std::vector<std::string> shaders{"./skybox.vert", "./skybox.frag"};
    skyboxP = new util::Program(shaders);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl environment", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window, glm::vec3(0.0, 0.0, 3.0));
    setup_mesh();
    setup_skybox();

    {
        std::vector<std::string> shaders{"./simple.vert", "./reflection.frag"};
        reflectP = new util::Program(shaders);
    }
    {
        std::vector<std::string> shaders{"./simple.vert", "./refraction.frag"};
        refractP = new util::Program(shaders);
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ctrl->handle_input();

        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pass 1: render object
        glBindVertexArray(meshVAO);
#if 0
        meshP->use();
        meshP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat() * glm::scale(glm::mat4(1.0), glm::vec3(0.85)));
        meshP->setMat4("view_mat", ctrl->get_view());
        meshP->setMat4("proj_mat", ctrl->get_proj());
        glm::vec3 light_loc = glm::vec3(ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));
        meshP->setVec3("light_loc", light_loc);
        meshP->setFloat("roughness", 32.0);
        mesh.draw(meshP);
#endif

#if 1
        reflectP->use();
        reflectP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat() * glm::scale(glm::mat4(1.0), glm::vec3(0.85)));
        reflectP->setMat4("view_mat", ctrl->get_view());
        reflectP->setMat4("proj_mat", ctrl->get_proj());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTEX);
        reflectP->setInt("Environment", 0);
        mesh.draw(reflectP);
#endif

#if 0
        refractP->use();
        refractP->setMat4("model_mat", ctrl->get_model() * mesh.get_model_mat() * glm::scale(glm::mat4(1.0), glm::vec3(0.85)));
        refractP->setMat4("view_mat", ctrl->get_view());
        refractP->setMat4("proj_mat", ctrl->get_proj());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTEX);
        refractP->setInt("Environment", 0);
        mesh.draw(refractP);
#endif

        // Pass 2: render skybox
        glBindVertexArray(skyboxVAO);
        skyboxP->use();
        // skyboxP->setMat4("model_mat", ctrl->get_model() * glm::scale(skybox.get_model_mat(), glm::vec3(4.0)));
        skyboxP->setMat4("model_mat", glm::scale(skybox.get_model_mat(), glm::vec3(4.0))); // pin skybox
        skyboxP->setMat4("view_mat", ctrl->get_view());
        skyboxP->setMat4("proj_mat", ctrl->get_proj());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTEX);
        skyboxP->setInt("Environment", 0);
        skybox.draw(skyboxP);

        glfwSwapBuffers(window);
    }

    delete refractP;
    delete reflectP;
    delete skyboxP;
    delete meshP;
    delete ctrl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
