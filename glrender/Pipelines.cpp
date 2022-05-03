#include "Common.hpp"
#include "Render.hpp"

void Render::BakeVAO() {
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

void Render::BakeDefaultPipeline() {
    std::vector<std::string> shaders { "./shaders/simple.vert", "./shaders/simple.frag" };
    progs.insert(std::make_pair("DEFAULT", new Program(shaders)));
}

void Render::run_if_default() {
    if (m_exclusive_mode != DEFAULT_MODE) {
        return;
    }

    Program *prog = progs["DEFAULT"];
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());
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
        if (obj.material_id != -1) {
            std::string diffuse_tex = mesh.m_materials[obj.material_id].diffuse_texname;

            if (mesh.m_textures.find(diffuse_tex) != mesh.m_textures.end()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh.m_textures[diffuse_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setInt("TEX0", 0);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
    }
}

void Render::BakeWireframePipeline() {
    std::vector<std::string> shaders { "./shaders/wireframe.vert", "./shaders/wireframe.frag" };
    progs.insert(std::make_pair("WIREFRAME", new Program(shaders)));
}

void Render::run_if_wireframe() {
    if (m_exclusive_mode != WIREFRAME_MODE) {
        return;
    }

    Program *prog = progs["WIREFRAME"];
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());
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

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(vao);
    for (auto & obj : mesh.m_objects) {
        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
    }
}

void Render::BakePhongPipeline() {
    std::vector<std::string> shaders { "./shaders/phong.vert", "./shaders/phong.frag" };
    progs.insert(std::make_pair("PHONG", new Program(shaders)));
}

void Render::run_if_phong() {
    if (m_exclusive_mode != PHONG_MODE) {
        return;
    }

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

    glBindVertexArray(vao);
    for (auto & obj : mesh.m_objects) {
        if (obj.material_id != -1) {
            // DIFFUSE
            std::string diffuse_tex = mesh.m_materials[obj.material_id].diffuse_texname;
            if (mesh.m_textures.find(diffuse_tex) != mesh.m_textures.end()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh.m_textures[diffuse_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setVec3("material.Kd", obj.material.Kd);
                prog->setInt("TEX0_DIFFUSE", 0);
            }
            // SPECULAR
            std::string specular_tex = mesh.m_materials[obj.material_id].specular_texname;
            if (mesh.m_textures.find(specular_tex) != mesh.m_textures.end()) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, mesh.m_textures[specular_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setVec3("material.Ks", obj.material.Ks);
                prog->setInt("TEX1_SPECULAR", 1);
            }
            // ROUGHNESS
            std::string roughness_tex = mesh.m_materials[obj.material_id].roughness_texname;
            if (mesh.m_textures.find(roughness_tex) != mesh.m_textures.end()) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, mesh.m_textures[roughness_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setInt("TEX2_ROUGHNESS", 2);
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
    }
}

void Render::BakeVVNPipeline() {
    std::vector<std::string> shaders { "./shaders/visualize_vertex_normal.vert",
        "./shaders/visualize_vertex_normal.geom",
        "./shaders/visualize_vertex_normal.frag" };
    progs.insert(std::make_pair("VISUALIZE_VERTEX_NORMAL", new Program(shaders)));
}

void Render::run_if_vvn() {
    if (m_exclusive_mode != VISUALIZE_VERTEX_NORMAL_MODE) {
        return;
    }

    Program *prog = progs["VISUALIZE_VERTEX_NORMAL"];
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());
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
        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
    }
}

void Render::BakeDiffuseSpecularPipeline() {
    std::vector<std::string> shaders { "./shaders/diffuse_specular.vert", "./shaders/diffuse_specular.frag" };
    progs.insert(std::make_pair("DIFFUSE_SPECULAR", new Program(shaders)));
}

void Render::run_if_diffuse_specular() {
    if (m_exclusive_mode != DIFFUSE_SPECULAR_MODE) {
        return;
    }

    Program *prog = progs["DIFFUSE_SPECULAR"];
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());

    glm::vec3 light_loc = glm::vec3(m_ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));
    prog->setVec3("light_loc", light_loc);
    prog->setFloat("roughness", m_roughness);
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

    glBindVertexArray(vao);
    for (auto & obj : mesh.m_objects) {
        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
    }
}

void Render::BakeToonPipeline() {
    std::vector<std::string> shaders { "./shaders/toon.vert", "./shaders/toon.frag" };
    progs.insert(std::make_pair("TOON", new Program(shaders)));
}

void Render::run_if_toon() {
    if (m_exclusive_mode != TOON_MODE) {
        return;
    }

    Program *prog = progs["TOON"];
    prog->setMat4("model_mat", m_ctrl->get_model() * mesh.get_model_mat());
    prog->setMat4("view_mat", m_ctrl->get_view());
    prog->setMat4("proj_mat", m_ctrl->get_proj());

    glm::vec3 light_loc = glm::vec3(m_ctrl->get_view() * glm::vec4(0.0, 0.0, 5.0, 1.0));
    prog->setVec3("light_loc[0]", light_loc);
    static float angle = 0.0;
    angle += 1000.0 / ImGui::GetIO().Framerate * 0.05;
    glm::vec3 orbit_light_loc = glm::vec3(m_ctrl->get_view() * glm::rotate(glm::mat4(1.0), glm::radians(angle), glm::vec3(0.0, 1.0, 0.0)) * glm::vec4(0.0, 0.0, 5.0, 1.0));
    prog->setVec3("light_loc[1]", orbit_light_loc);
    prog->use();

    GLuint vao = vaos["GENERAL"];

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.98, 0.90, 0.68, 0.0);
    glClearDepthf(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glBindVertexArray(vao);
    for (auto & obj : mesh.m_objects) {
        if (obj.material_id != -1) {
            // DIFFUSE
            std::string diffuse_tex = mesh.m_materials[obj.material_id].diffuse_texname;
            if (mesh.m_textures.find(diffuse_tex) != mesh.m_textures.end()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh.m_textures[diffuse_tex]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                prog->setVec3("material.Kd", obj.material.Kd);
                prog->setInt("TEX0_DIFFUSE", 0);
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, obj.buffer_id);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertices.size());
    }
}