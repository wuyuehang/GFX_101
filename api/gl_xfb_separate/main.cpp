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

#define _query_xfb_prim_ 1
util::AssimpMesh mesh;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1024, 768, "gl xfb separate", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    util::Controller *ctrl = new util::TrackballController(window);

    // Pass 1 resource
    GLuint xfbVAO;
    glGenVertexArrays(1, &xfbVAO);
    glBindVertexArray(xfbVAO);
    mesh.load("../../assets/gltf/metal_cup_ww2_style_cup_vintage/scene.gltf");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AdvVertex), (const void*)offsetof(AdvVertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    std::vector<const char *> xfb_list;
    xfb_list.push_back("xfbPos");
    xfb_list.push_back("xfbNor");
    xfb_list.push_back("xfbUV");
    GLuint xfbP = gltest::CreateXFBProgram("./xfb.vert", xfb_list, GL_SEPARATE_ATTRIBS);

    GLuint XFB[3];
    glGenBuffers(3, XFB);
    // xfbPos
    glBindBuffer(GL_ARRAY_BUFFER, XFB[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh.get_index_num() * sizeof(Vertex::pos), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, XFB[0]);
    // xfbNor
    glBindBuffer(GL_ARRAY_BUFFER, XFB[1]);
    glBufferData(GL_ARRAY_BUFFER, mesh.get_index_num() * sizeof(Vertex::nor), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, XFB[1]);
    // xfbUV
    glBindBuffer(GL_ARRAY_BUFFER, XFB[2]);
    glBufferData(GL_ARRAY_BUFFER, mesh.get_index_num() * sizeof(Vertex::uv), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, XFB[2]);

#if _query_xfb_prim_
    GLuint xfb_query;
    glGenQueries(1, &xfb_query);
#endif

    // Pass 2 resource
    GLuint rsVAO;
    glGenVertexArrays(1, &rsVAO);
    glBindVertexArray(rsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, XFB[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex::pos), (const void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, XFB[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex::nor), (const void*)0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, XFB[2]);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex::uv), (const void*)0);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    std::vector<std::string> shaders { "./simple.vert", "./simple.frag" };
    GLuint rsP = gltest::CreateProgram(shaders);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ctrl->handle_input();

        glViewport(0, 0, 1024, 768);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2, 0.2, 0.3, 1.0);
        glClearDepthf(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pass 1. generate xfb in view space
        glBindVertexArray(xfbVAO);
        glUseProgram(xfbP);
        glUniformMatrix4fv(glGetUniformLocation(xfbP, "model_mat"), 1, GL_FALSE, &ctrl->get_model()[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(xfbP, "view_mat"), 1, GL_FALSE, &ctrl->get_view()[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(xfbP, "proj_mat"), 1, GL_FALSE, &ctrl->get_proj()[0][0]);
        assert(mesh.m_objects.size() == 1); // assume the mesh only contains 1 mesh.

#if _query_xfb_prim_
        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, xfb_query);
#endif
        glEnable(GL_RASTERIZER_DISCARD);
        glBeginTransformFeedback(GL_TRIANGLES);
        for (auto & obj : mesh.m_objects) {
            glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, (const void *)(obj.firstIndex * sizeof(GLuint)));
        }
        glEndTransformFeedback();
#if _query_xfb_prim_
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        GLuint num_prim;
        glGetQueryObjectuiv(xfb_query, GL_QUERY_RESULT, &num_prim);
        std::cout << "mesh contains " << mesh.get_index_num() / 3 << " primitives" << std::endl;
        std::cout << "xfb writes " << num_prim << " primitives" << std::endl;
#endif
        // Pass 2. draw the xfb
        glBindVertexArray(rsVAO);
        glUseProgram(rsP);
        glUniformMatrix4fv(glGetUniformLocation(rsP, "proj_mat"), 1, GL_FALSE, &ctrl->get_proj()[0][0]);
        glDisable(GL_RASTERIZER_DISCARD);
        if (!mesh.m_objects[0].material_names.diffuse_texname.empty()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.m_textures[mesh.m_objects[0].material_names.diffuse_texname]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glUniform1i(glGetUniformLocation(rsP, "TEX0_DIFFUSE"), 0);
        }
        glDrawArrays(GL_TRIANGLES, 0, mesh.get_index_num()); // note that, we are flatting the vertex buffer
        glfwSwapBuffers(window);
    }

    delete ctrl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
