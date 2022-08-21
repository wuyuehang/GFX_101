#ifndef SKELETON_VULKAN_HPP
#define SKELETON_VULKAN_HPP
#include "VulkanCore.hpp"
#include "VulkanSwapchain.hpp"

namespace common {

class SkeletonVulkan : public VulkanCore {
public:
    SkeletonVulkan(const SkeletonVulkan &) = delete;
    SkeletonVulkan &operator=(const SkeletonVulkan &) = delete;
    SkeletonVulkan(uint32_t version, int32_t width=16, int32_t height=16) : m_api_version(version), m_width(width), m_height(height) {}
    void init() {
        VkApplicationInfo appInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "SkeletonVulkan",
            .applicationVersion = 0,
            .pEngineName = "SkeletonVulkan",
            .engineVersion = 0,
            .apiVersion = m_api_version,
        };
        std::array<const char *, 2> instance_extension { "VK_KHR_surface", "VK_KHR_xcb_surface" };
        VkInstanceCreateInfo instInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = instance_extension.size(),
            .ppEnabledExtensionNames = instance_extension.data(),
        };
        vkCreateInstance(&instInfo, nullptr, &instance);

        uint32_t nof;
        vkEnumeratePhysicalDevices(instance, &nof, nullptr);
        pdev.resize(nof);
        vkEnumeratePhysicalDevices(instance, &nof, pdev.data());

        for (auto it : pdev) {
            VkPhysicalDeviceProperties prop {};
            vkGetPhysicalDeviceProperties(it, &prop);
            std::cout << "vendor " << std::hex << prop.vendorID << ", name " << prop.deviceName << std::endl;

            uint32_t nof;
            vkGetPhysicalDeviceQueueFamilyProperties(it, &nof, nullptr);
            std::vector<VkQueueFamilyProperties> queue_prop(nof);
            vkGetPhysicalDeviceQueueFamilyProperties(it, &nof, queue_prop.data());
        }

        const float qp = 1.0;
        VkDeviceQueueCreateInfo queueInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .pQueuePriorities = &qp,
        };
        std::array<const char *, 1> dev_ext { "VK_KHR_swapchain" };
        VkDeviceCreateInfo deviceInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = dev_ext.size(),
            .ppEnabledExtensionNames = dev_ext.data(),
            .pEnabledFeatures = nullptr,
        };
        vkCreateDevice(pdev[0], &deviceInfo, nullptr, &dev);
        vkGetDeviceQueue(dev, 0, 0, &queue);
        vkGetPhysicalDeviceMemoryProperties(pdev[0], &mem_properties);

        {
            VkCommandPoolCreateInfo cmdpoolInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = 0,
            };
            vkCreateCommandPool(dev, &cmdpoolInfo, nullptr, &cmdpool);

            VkCommandBufferAllocateInfo allocInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = cmdpool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            vkAllocateCommandBuffers(dev, &allocInfo, &transfer_cmdbuf);
        }
        {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            window = glfwCreateWindow(m_width, m_height, "SkeletonVulkan", nullptr, nullptr);
            swapchain = new VulkanSwapchain(this, window);
            swapchain->init();
        }
    }
    ~SkeletonVulkan() {
        swapchain->deinit();
        delete swapchain;
        glfwDestroyWindow(window);
        glfwTerminate();
        vkFreeCommandBuffers(dev, cmdpool, 1, &transfer_cmdbuf);
        vkDestroyCommandPool(dev, cmdpool, nullptr);
        vkDestroyDevice(dev, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
    VkInstance get_instance() const override final { return instance; }
    VkPhysicalDevice get_pdev() const override final { return pdev[0]; }
    VkDevice get_dev() const override final { return dev; }
    VkQueue get_queue() const override final { return queue; }
    VkPhysicalDeviceMemoryProperties get_mem_properties() const override final { return mem_properties; }
    VkCommandBuffer get_transfer_cmdbuf() const override final { return transfer_cmdbuf; }
protected:
    uint32_t m_api_version;
    VkInstance instance;
    std::vector<VkPhysicalDevice> pdev;
    VkDevice dev;
    VkQueue queue;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkCommandPool cmdpool;
    VkCommandBuffer transfer_cmdbuf;
    GLFWwindow *window;
    int32_t m_width; int32_t m_height;
    VulkanSwapchain *swapchain;
    VkSemaphore image_available_sema;
    VkSemaphore image_render_finished_sema;
};

} // namespace common

#endif
