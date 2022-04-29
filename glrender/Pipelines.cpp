#include "Common.hpp"
#include "Render.hpp"

void Render::BakeDefaultPipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "DEFAULT", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/simple.vert", "./shaders/simple.frag" };
    progs.insert({ "DEFAULT", new Program(shaders) });
}

void Render::run_if_default(GLuint VBO) {
    if (m_exclusive_mode != DEFAULT_MODE) {
        return;
    }

    Program *prog = progs.find("DEFAULT")->second;
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TEX);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    prog->setInt("TEX0", 0);
    prog->use();

    GLuint vao = vaos.find("DEFAULT")->second;

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepthf(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
}

void Render::BakeWireframePipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "WIREFRAME", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/wireframe.vert", "./shaders/wireframe.frag" };
    progs.insert({ "WIREFRAME", new Program(shaders) });
}

void Render::run_if_wireframe(GLuint VBO) {
    if (m_exclusive_mode != WIREFRAME_MODE) {
        return;
    }

    Program *prog = progs.find("WIREFRAME")->second;
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());
    prog->use();

    GLuint vao = vaos.find("WIREFRAME")->second;

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepthf(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
}

void Render::BakePhongPipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "PHONG", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/phong.vert", "./shaders/phong.frag" };
    progs.insert({ "PHONG", new Program(shaders) });
}

void Render::run_if_phong(GLuint VBO) {
    if (m_exclusive_mode != PHONG_MODE) {
        return;
    }

    Program *prog = progs.find("PHONG")->second;
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());

    glm::vec3 light_loc = glm::vec3(m_ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));
    prog->setVec3("light_loc", light_loc);
    prog->setFloat("roughness", m_roughness);
    prog->use();

    GLuint vao = vaos.find("PHONG")->second;

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepthf(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
}

void Render::BakeVVNPipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "VISUALIZE_VERTEX_NORMAL", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/visualize_vertex_normal.vert",
        "./shaders/visualize_vertex_normal.geom",
        "./shaders/visualize_vertex_normal.frag" };
    progs.insert({ "VISUALIZE_VERTEX_NORMAL", new Program(shaders) });
}

void Render::run_if_vvn(GLuint VBO) {
    if (m_exclusive_mode != VISUALIZE_VERTEX_NORMAL_MODE) {
        return;
    }

    Program *prog = progs.find("VISUALIZE_VERTEX_NORMAL")->second;
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());
    prog->use();

    GLuint vao = vaos.find("VISUALIZE_VERTEX_NORMAL")->second;

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepthf(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertices().size());
}