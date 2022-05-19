#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Mesh.hpp"

GLuint VAO, VS, FS, PPL;
util::AssimpMesh mesh;
glm::mat4 model_mat;
glm::mat4 view_mat = glm::lookAt(glm::vec3(0.0, 0.0, 3.0), glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));
glm::mat4 proj_mat = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

static void loadGLSL(const std::string filename, std::string & code) {
    std::string line;
    std::ifstream f(filename);
    assert(f.is_open());

    while (std::getline(f, line)) {
        code.append(line);
        code.append(1, '\n');
    }
    f.close();
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl separate shader", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    glViewport(0, 0, 1024, 768);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::vector<std::string> shaders { "./uniform_block.vert", "./simple.frag" };
    std::string code;
    loadGLSL("./simple.vert", code);
    const GLchar *tok = code.c_str();
    VS = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &tok);

    code.clear();
    loadGLSL("./simple.frag", code);
    tok = code.c_str();
    FS = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &tok);

    glGenProgramPipelines(1, &PPL);
    glBindProgramPipeline(PPL);
    glUseProgramStages(PPL, GL_VERTEX_SHADER_BIT, VS);
    glUseProgramStages(PPL, GL_FRAGMENT_SHADER_BIT, FS);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    mesh.load("../../assets/obj/torus.obj");
    model_mat = mesh.get_model_mat();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, uv));
    glEnableVertexAttribArray(2);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glProgramUniformMatrix4fv(VS, glGetUniformLocation(VS, "model_mat"), 1, GL_FALSE, &model_mat[0][0]);
        glProgramUniformMatrix4fv(VS, glGetUniformLocation(VS, "view_mat"), 1, GL_FALSE, &view_mat[0][0]);
        glProgramUniformMatrix4fv(VS, glGetUniformLocation(VS, "proj_mat"), 1, GL_FALSE, &proj_mat[0][0]);

        for (auto & obj : mesh.m_objects) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, obj.indices.size(), GL_UNSIGNED_INT, 0);
        }
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
