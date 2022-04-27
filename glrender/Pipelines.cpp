#include "Common.hpp"
#include "Render.hpp"

void Render::BakeDefaultPipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "DEFAULT", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/simple.vert", "./shaders/simple.frag" };
    progs.insert({ "DEFAULT", new Program(shaders) });
}

void Render::BakeWireframePipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "WIREFRAME", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/wireframe.vert", "./shaders/wireframe.frag" };
    progs.insert({ "WIREFRAME", new Program(shaders) });
}

void Render::BakePhongPipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "PHONG", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/phong.vert", "./shaders/phong.frag" };
    progs.insert({ "PHONG", new Program(shaders) });
}

void Render::BakeVVNPipeline(GLuint VBO) {
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, nor));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (const void*)offsetof(Mesh::Vertex, uv));
    glEnableVertexAttribArray(2);

    vaos.insert({ "VISUALIZE_VERTEX_NORMAL", VAO });

    glBindVertexArray(0);

    std::vector<std::string> shaders { "./shaders/visualize_vertex_normal.vert",
        "./shaders/visualize_vertex_normal.geom",
        "./shaders/visualize_vertex_normal.frag" };
    progs.insert({ "VISUALIZE_VERTEX_NORMAL", new Program(shaders) });
}