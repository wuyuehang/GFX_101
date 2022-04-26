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

    vaos.insert({"DEFAULT", VAO});

    glBindVertexArray(0);

    GLuint VS = BuildShader("./shaders/simple.vert", GL_VERTEX_SHADER);
    GLuint FS = BuildShader("./shaders/simple.frag", GL_FRAGMENT_SHADER);
    std::vector<GLuint> shaders { VS, FS };
    GLuint prog = BuildProgram(shaders);
    programs.insert({ "DEFAULT", prog });
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

    vaos.insert({"WIREFRAME", VAO});

    glBindVertexArray(0);

    GLuint VS = BuildShader("./shaders/wireframe.vert", GL_VERTEX_SHADER);
    GLuint FS = BuildShader("./shaders/wireframe.frag", GL_FRAGMENT_SHADER);
    std::vector<GLuint> shaders { VS, FS };
    GLuint prog = BuildProgram(shaders);
    programs.insert({ "WIREFRAME", prog });
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

    vaos.insert({"PHONG", VAO});

    glBindVertexArray(0);

    GLuint VS = BuildShader("./shaders/phong.vert", GL_VERTEX_SHADER);
    GLuint FS = BuildShader("./shaders/phong.frag", GL_FRAGMENT_SHADER);
    std::vector<GLuint> shaders { VS, FS };
    GLuint prog = BuildProgram(shaders);
    programs.insert({ "PHONG", prog });
}