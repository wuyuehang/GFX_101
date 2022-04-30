#ifndef HELLO_VULKAN_HPP
#define HELLO_VULKAN_HPP

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Controller.hpp"
#include "Mesh.hpp"
#include "VulkanCommon.hpp"
#include "VulkanSwapchain.hpp"

#define APP_NAME "HelloVulkan"

class VulkanSwapchain;

class HelloVulkan {
public:
    struct VulkanPipe {
        VkDescriptorPool pool;
        std::vector<VkDescriptorSet> ds;
        /* A blueprint describes the shader binding layout (without actually referencing descriptor)
         * can be used with different descriptor sets as long as their layout matches
         */
        VkDescriptorSetLayout dsl;
        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;
    };
    HelloVulkan();
    ~HelloVulkan();
    void InitGLFW();
    void CreateInstance();
    void CreateDevice();
    void InitSync();
    void CreateCommand();
    void CreateRenderPass();
    void CreateFramebuffer();
    void bake_imgui();
    void CreateResource();
    /* axis pipleine */
    void bake_axis_DescriptorSetLayout(VulkanPipe &);
    void bake_axis_DescriptorSet(VulkanPipe &);
    void bake_axis_Pipeline(VulkanPipe &);
    /* default pipleine */
    void bake_default_DescriptorSetLayout(VulkanPipe &);
    void bake_default_DescriptorSet(VulkanPipe &);
    void bake_default_Pipeline(VulkanPipe &);
    void run_if_default(VulkanPipe &, uint32_t);
    /* wireframe pipeline */
    void bake_wireframe_DescriptorSetLayout(VulkanPipe &);
    void bake_wireframe_DescriptorSet(VulkanPipe &);
    void bake_wireframe_Pipeline(VulkanPipe &);
    void run_if_wireframe(VulkanPipe &, uint32_t);
    /* visualize vertex normal pipeline */
    void bake_visualize_vertex_normal_DescriptorSetLayout(VulkanPipe &);
    void bake_visualize_vertex_normal_DescriptorSet(VulkanPipe &);
    void bake_visualize_vertex_normal_Pipeline(VulkanPipe &);
    void run_if_vnn(VulkanPipe &, uint32_t);
    /* phong pipeline */
    void bake_phong_DescriptorSetLayout(VulkanPipe &);
    void bake_phong_DescriptorSet(VulkanPipe &);
    void bake_phong_Pipeline(VulkanPipe &);
    void run_if_phong(VulkanPipe &, uint32_t);
    void clean_VulkanPipe(VulkanPipe p);
    void begin_command_buffer(VkCommandBuffer &cmd);
    void end_command_buffer(VkCommandBuffer &cmd);
    void BakeCommand(uint32_t frame_nr);
    void HandleInput();
    void Gameloop();

public:
    VkInstance instance;
    VkPhysicalDevice pdev;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkDevice dev;
    VkQueue queue;
    VkCommandBuffer transfer_cmdbuf;
    GLFWwindow *window;
    int32_t w;
    int32_t h;

private:
    Controller *m_ctrl;
    VulkanSwapchain vulkan_swapchain;
    uint32_t q_family_index;
    uint32_t m_max_inflight_frames;
    uint32_t m_current_frame;
    /* sync objects */
    std::vector<VkSemaphore> image_available;
    std::vector<VkSemaphore> image_render_finished;
    std::vector<VkFence> fence;
    VkCommandPool cmdpool;
    std::vector<VkCommandBuffer> cmdbuf;
    VkRenderPass rp;
    std::vector<VkFramebuffer> framebuffers;
    ImageObj *depth;
    /* various pipeline */
    VulkanPipe axis_pipe;
    VulkanPipe default_pipe;
    VulkanPipe wireframe_pipe;
    VulkanPipe visualize_vertex_normal_pipe;
    VulkanPipe phong_pipe;
    /* axis */
    Mesh axis_mesh;
    BufferObj *axis_vertex;
    std::vector<BufferObj *> axis_mvp_uniform;
    /* default resource */
    Mesh default_mesh;
    BufferObj *default_vertex;
    ImageObj *default_tex;
    VkSampler default_sampler;
    std::vector<BufferObj *> default_mvp_uniform;
    /* scene */
    std::vector<BufferObj *> scene_uniform;
    /* imgui */
    VkDescriptorPool imgui_pool;
    enum {
        DEFAULT_MODE = 0,
        WIREFRAME_MODE,
        VISUALIZE_VERTEX_NORMAL_MODE,
        PHONG_MODE,
    };
    int m_exclusive_mode;
    float m_roughness;
    bool m_display_axis;
};
#endif