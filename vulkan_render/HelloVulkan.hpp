#ifndef HELLO_VULKAN_HPP
#define HELLO_VULKAN_HPP

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>

#include "Controller.hpp"
#include "VulkanCommon.hpp"
#include "VulkanSwapchain.hpp"

#define APP_NAME "HelloVulkan"

class VulkanSwapchain;

class HelloVulkan {
public:
    HelloVulkan();
    ~HelloVulkan();
    void InitGLFW() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    #if 0
        GLFWmonitor *prim = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(prim);
        w = mode->width;
        h = mode->height;
        window = glfwCreateWindow(w, h, APP_NAME, prim, nullptr);
        glfwSetWindowMonitor(window, prim, 0, 0, w, h, mode->refreshRate);
    #else
        w = 1024;
        h = 1024;
        window = glfwCreateWindow(w, h, APP_NAME, nullptr, nullptr);
    #endif
    }
    void CreateInstance() {
        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pApplicationName = APP_NAME;
        app_info.pEngineName = APP_NAME;
        app_info.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_info.pApplicationInfo = &app_info;
        std::array<const char *, 2> instance_extension { "VK_KHR_surface", "VK_KHR_xcb_surface" };
        instance_info.ppEnabledExtensionNames = instance_extension.data();
        instance_info.enabledExtensionCount = instance_extension.size();
        vkCreateInstance(&instance_info, nullptr, &instance);
    }
    void CreateDevice() {
        uint32_t pdev_count;
        vkEnumeratePhysicalDevices(instance, &pdev_count, nullptr);
        std::vector<VkPhysicalDevice> physical_dev(pdev_count);
        vkEnumeratePhysicalDevices(instance, &pdev_count, physical_dev.data());
        pdev = physical_dev[0];

        uint32_t queue_prop_count;
        q_family_index = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queue_prop_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_props(queue_prop_count);
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queue_prop_count, queue_props.data());
        for (auto& it : queue_props) {
            if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT && it.queueFlags & VK_QUEUE_COMPUTE_BIT && it.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                break;
            }
            q_family_index++;
        }

        VkDeviceQueueCreateInfo queue_info { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        float q_prio = 1.0;
        queue_info.queueFamilyIndex = q_family_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &q_prio;

        VkDeviceCreateInfo dev_info { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        std::array<const char *, 1> dev_ext { "VK_KHR_swapchain" };
        dev_info.queueCreateInfoCount = 1;
        dev_info.pQueueCreateInfos = &queue_info;
        dev_info.enabledExtensionCount = dev_ext.size();
        dev_info.ppEnabledExtensionNames = dev_ext.data();

        vkCreateDevice(pdev, &dev_info, nullptr, &dev);
        vkGetDeviceQueue(dev, q_family_index, 0, &queue);

        vkGetPhysicalDeviceMemoryProperties(pdev, &mem_properties);
    }
    void InitSync() {
        m_max_inflight_frames = vulkan_swapchain.images.size();
        VkSemaphoreCreateInfo sema_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // mark as signed upon creation

        image_available.resize(m_max_inflight_frames);
        image_render_finished.resize(m_max_inflight_frames);
        fence.resize(m_max_inflight_frames);

        for (auto i = 0; i < m_max_inflight_frames; i++) {
            vkCreateSemaphore(dev, &sema_info, nullptr, &image_available[i]);
            vkCreateSemaphore(dev, &sema_info, nullptr, &image_render_finished[i]);
            vkCreateFence(dev, &fence_info, nullptr, &fence[i]);
        }
    }
    void CreateCommand();
    void CreateRenderPass();
    void CreateFramebuffer();
    void CreateResource();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSet();
    void CreatePipeline();
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
    Controller *ctrl;
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
    VkPipelineLayout layout;
    VkPipeline pipeline;
    /* A blueprint describes the shader binding layout (without actually referencing descriptor)
     * can be used with different descriptor sets as long as their layout matches
     */
    VkDescriptorSetLayout dsl;
    VkDescriptorPool pool;
    std::vector<VkDescriptorSet> ds;
    BufferObj *index;
    BufferObj *vertex;
    std::vector<BufferObj *> uniform;
    ImageObj *depth;
    ImageObj *tex;
    VkSampler sampler;
};
#endif