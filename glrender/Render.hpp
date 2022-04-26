#ifndef RENDER_HPP
#define RENDER_HPP

#undef GLRENDER_DEBUG
#define GLRENDER_DEBUG 1

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Mesh.hpp"

class Controller;

class Render {
public:
    Render(const Render &) = delete;
    Render();
    ~Render();
    void InitGLFW();
    void InitImGui();
    void CreateResource();
    void BakeDefaultPipeline(GLuint VBO);
    void BakeWireframePipeline(GLuint VBO);
    void BakeCommand();
    void Gameloop();

public:
    GLFWwindow *m_window;
    int32_t m_width;
    int32_t m_height;
private:
    enum {
        DEFAULT_MODE = 0,
        WIREFRAME_MODE,
        VISUALIZE_VERTEX_NORMAL_MODE,
        PHONG_MODE,
    };
    int exclusive_mode;
    Controller *ctrl;
    GLuint UBO;
    GLuint VBO;
    Mesh mesh;
    std::map<std::string, GLuint> vaos;
    std::map<std::string, GLuint> programs;
};
#endif