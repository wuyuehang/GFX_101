#ifndef ES_RENDER_HPP
#define ES_RENDER_HPP

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Program.hpp"
#include <map>

class Render {
public:
    Render(const Render &) = delete;
    Render();
    ~Render();
    void InitGLFW();
    void InitImGui();
    void CreateResource();
    void BakeDefaultPipeline(GLuint VBO);
    void run_if_default(GLuint VBO);
    void BakeVVNPipeline(GLuint VBO);
    void run_if_vnn(GLuint VBO);
    void BakePhongPipeline(GLuint VBO);
    void run_if_phong(GLuint VBO);
    void BakeCommand();
    void Gameloop();

public:
    GLFWwindow *m_window;
    int32_t m_width;
    int32_t m_height;
private:
    enum {
        DEFAULT_MODE = 0,
        VISUALIZE_VERTEX_NORMAL_MODE,
        PHONG_MODE,
    };
    int m_exclusive_mode;
    Controller *m_ctrl;
    GLuint VBO;
    Mesh mesh;
    float m_roughness;
    std::map<std::string, GLuint> vaos;
    std::map<std::string, Program *> progs;
};

#endif