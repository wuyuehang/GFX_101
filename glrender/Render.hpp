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
#include "Controller.hpp"
#include "Program.hpp"

class Render {
public:
    Render(const Render &) = delete;
    Render();
    ~Render();
    void InitGLFW();
    void InitImGui();
    void CreateResource();
    void BakeDefaultPipeline();
    void run_if_default();
    void BakeWireframePipeline();
    void run_if_wireframe();
    void BakeVVNPipeline();
    void run_if_vvn();
    void BakePhongPipeline();
    void run_if_phong();
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
    int m_exclusive_mode;
    Controller *m_ctrl;
    Mesh mesh;
    float m_roughness;
    std::map<std::string, GLuint> vaos;
    std::map<std::string, Program *> progs;
};
#endif
