#ifndef SKELETON_VULKAN_HPP
#define SKELETON_VULKAN_HPP
#include "VulkanCore.hpp"

namespace common {

class SkeletonVulkan : public VulkanCore {
public:
    SkeletonVulkan(const SkeletonVulkan &) = delete;
    SkeletonVulkan &operator=(const SkeletonVulkan &) = delete;
    SkeletonVulkan(uint32_t version) {
        VkApplicationInfo appInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "SkeletonVulkan",
            .applicationVersion = 0,
            .pEngineName = "SkeletonVulkan",
            .engineVersion = 0,
            .apiVersion = version,
        };
        VkInstanceCreateInfo instInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = nullptr,
        };
        vkCreateInstance(&instInfo, nullptr, &instance);

        uint32_t nof;
        vkEnumeratePhysicalDevices(instance, &nof, nullptr);
        std::vector<VkPhysicalDevice> pdev(nof);
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

        VkDeviceCreateInfo deviceInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = nullptr,
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
    }
    ~SkeletonVulkan() {
        vkFreeCommandBuffers(dev, cmdpool, 1, &transfer_cmdbuf);
        vkDestroyCommandPool(dev, cmdpool, nullptr);
        vkDestroyDevice(dev, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
    VkDevice get_dev() const override final { return dev; }
    VkQueue get_queue() const override final { return queue; }
    VkPhysicalDeviceMemoryProperties get_mem_properties() const override final { return mem_properties; }
    VkCommandBuffer get_transfer_cmdbuf() const override final { return transfer_cmdbuf; }
protected:
    VkInstance instance;
    VkDevice dev;
    VkQueue queue;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkCommandPool cmdpool;
    VkCommandBuffer transfer_cmdbuf;
};

} // namespace common

#endif
