#ifndef SKELETON_COMPUTE_VULKAN_HPP
#define SKELETON_COMPUTE_VULKAN_HPP
#include "VulkanCore.hpp"

namespace common {

class SkeletonComputeVulkan : public VulkanCore {
public:
    SkeletonComputeVulkan(const SkeletonComputeVulkan &) = delete;
    SkeletonComputeVulkan &operator=(const SkeletonComputeVulkan &) = delete;
    SkeletonComputeVulkan(uint32_t version) : m_api_version(version) {}
    void init() override final {
        VkApplicationInfo app_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "SkeletonComputeVulkan",
            .applicationVersion = 0,
            .pEngineName = "SkeletonComputeVulkan",
            .engineVersion = 0,
            .apiVersion = m_api_version,
        };
        VkInstanceCreateInfo inst_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = (uint32_t)m_instance_extension.size(),
            .ppEnabledExtensionNames = m_instance_extension.data(),
        };
        vkCreateInstance(&inst_info, nullptr, &instance);

        uint32_t nof;
        vkEnumeratePhysicalDevices(instance, &nof, nullptr);
        pdev.resize(nof);
        vkEnumeratePhysicalDevices(instance, &nof, pdev.data());

        for (const auto it : pdev) {
            VkPhysicalDeviceProperties prop {};
            vkGetPhysicalDeviceProperties(it, &prop);
            std::cout << "vendor " << std::hex << prop.vendorID << ", name " << prop.deviceName << std::endl;

            uint32_t nof;
            vkGetPhysicalDeviceQueueFamilyProperties(it, &nof, nullptr);
            std::vector<VkQueueFamilyProperties> queue_prop(nof);
            vkGetPhysicalDeviceQueueFamilyProperties(it, &nof, queue_prop.data());
        }

        const float qp = 1.0;
        VkDeviceQueueCreateInfo queue_info {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .pQueuePriorities = &qp,
        };

        VkDeviceCreateInfo device_info {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = (uint32_t)m_device_extension.size(),
            .ppEnabledExtensionNames = m_device_extension.data(),
            .pEnabledFeatures = nullptr,
        };

        VkPhysicalDeviceFeatures2 feature2 {};
        if (m_device_info_next) {
            feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            feature2.pNext = m_device_info_next;
            //feature2.features = nullptr;// core feature
            device_info.pNext = &feature2;
            device_info.pEnabledFeatures = nullptr;
        }
        vkCreateDevice(pdev[0], &device_info, nullptr, &dev);
        vkGetDeviceQueue(dev, 0, 0, &queue);
        vkGetPhysicalDeviceMemoryProperties(pdev[0], &mem_properties);

        {
            VkCommandPoolCreateInfo cmdpool_info {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = 0,
            };
            vkCreateCommandPool(dev, &cmdpool_info, nullptr, &cmdpool);

            VkCommandBufferAllocateInfo alloc_info {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = cmdpool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            vkAllocateCommandBuffers(dev, &alloc_info, &transfer_cmdbuf);
        }
    }
    ~SkeletonComputeVulkan() {
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
    std::vector<const char *> m_instance_extension;
    std::vector<const char *> m_device_extension;
    void *m_device_info_next = nullptr;
    VkInstance instance;
    std::vector<VkPhysicalDevice> pdev;
    VkDevice dev;
    VkQueue queue;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkCommandPool cmdpool;
    VkCommandBuffer transfer_cmdbuf;
};

} // namespace common

#endif
