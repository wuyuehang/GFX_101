#include "Common.hpp"
#include "PBRRender.hpp"

void PBRRender::BakeVAO() {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert(std::make_pair("GENERAL", VAO));
}

void PBRRender::BakeDefaultPipeline() {
    std::vector<std::string> shaders { "./shaders/simple.vert", "./shaders/simple.frag" };
    progs.insert(std::make_pair("DEFAULT", new Program(shaders)));
}

void PBRRender::run_if_default() {
    if (m_exclusive_mode != DEFAULT_MODE) {
        return;
    }

    Program *prog = progs["DEFAULT"];
    prog->setMat4("view_mat", m_ctrl->get_view() * m_ctrl->get_model());
    prog->setMat4("proj_mat", m_ctrl->get_proj());
    prog->use();

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

    glBindVertexArray(vaos["GENERAL"]);

    prog->setInt("TEX0", 0);
    mesh.draw(prog);
}

void PBRRender::BakeWireframePipeline() {
    std::vector<std::string> shaders { "./shaders/wireframe.vert", "./shaders/wireframe.frag" };
    progs.insert(std::make_pair("WIREFRAME", new Program(shaders)));
}

void PBRRender::run_if_wireframe() {
    if (m_exclusive_mode != WIREFRAME_MODE) {
        return;
    }

    Program *prog = progs["WIREFRAME"];
    prog->setMat4("view_mat", m_ctrl->get_view() * m_ctrl->get_model()); // aggregate controller's View * Model transform into View
    prog->setMat4("proj_mat", m_ctrl->get_proj());
    prog->use();
    glBindVertexArray(vaos["GENERAL"]);

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

    mesh.draw(prog);
}

void PBRRender::BakePhongPipeline() {
    std::vector<std::string> shaders { "./shaders/phong.vert", "./shaders/phong.frag" };
    progs.insert(std::make_pair("PHONG", new Program(shaders)));
}

void PBRRender::run_if_phong() {
    if (m_exclusive_mode != PHONG_MODE) {
        return;
    }
/*
    Program *prog = progs["PHONG"];
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());

    glm::vec3 light_loc = glm::vec3(m_ctrl->get_view() * glm::vec4(0.0, 0.0, 5.0, 1.0));
    prog->setVec3("light_loc[0]", light_loc);
    static float angle = 0.0;
    angle += 1000.0 / ImGui::GetIO().Framerate * 0.05;
    glm::vec3 orbit_light_loc = glm::vec3(m_ctrl->get_view() * glm::rotate(glm::mat4(1.0), glm::radians(angle), glm::vec3(0.0, 1.0, 0.0)) * glm::vec4(0.0, 0.0, 5.0, 1.0));
    prog->setVec3("light_loc[1]", orbit_light_loc);

    prog->setFloat("attenuation.Kc", 1.0);
    prog->setFloat("attenuation.Kl", 0.09);
    prog->setFloat("attenuation.Kq", 0.032);
    prog->use();

    GLuint vao = vaos["GENERAL"];

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

    for (auto & obj : mesh.m_objects) {
        mesh.bind_diffuse(obj, 0);
        prog->setVec3("material.Kd", obj.material.Kd);
        prog->setInt("TEX0_DIFFUSE", 0);

        mesh.bind_specular(obj, 1);
        prog->setVec3("material.Ks", obj.material.Ks);
        prog->setInt("TEX1_SPECULAR", 1);

        mesh.bind_roughness(obj, 2);
        prog->setInt("TEX2_ROUGHNESS", 2);
        mesh.draw(obj);
    }
    */
}

void PBRRender::BakeDiffuseSpecularPipeline() {
    std::vector<std::string> shaders { "./shaders/diffuse_specular.vert", "./shaders/diffuse_specular.frag" };
    progs.insert(std::make_pair("DIFFUSE_SPECULAR", new Program(shaders)));
}

void PBRRender::run_if_diffuse_specular() {
    if (m_exclusive_mode != DIFFUSE_SPECULAR_MODE) {
        return;
    }

    Program *prog = progs["DIFFUSE_SPECULAR"];
    prog->setMat4("view_mat", m_ctrl->get_view() * m_ctrl->get_model());
    prog->setMat4("proj_mat", m_ctrl->get_proj());

    glm::vec3 light_loc = glm::vec3(m_ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));
    prog->setVec3("light_loc", light_loc);
    prog->setFloat("roughness", m_roughness);
    prog->use();

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
    glBindVertexArray(vaos["GENERAL"]);

    mesh.draw(prog);
}