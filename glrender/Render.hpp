#ifndef RENDER_HPP
#define RENDER_HPP

#undef GLRENDER_DEBUG
#define GLRENDER_DEBUG 1

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

class Controller;

class Render {
public:
    Render(const Render &) = delete;
    Render();
    ~Render();
    void InitGLFW();
    void InitImGui();
    void CreateResource();
    void BakeCommand();
    void Gameloop();

public:
    GLFWwindow *m_window;
    int32_t m_width;
    int32_t m_height;
private:
    Controller *ctrl;
    GLuint UBO;
    GLuint VS;
};
#endif