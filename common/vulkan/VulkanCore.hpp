#ifndef VULKAN_CORE_HPP
#define VULKAN_CORE_HPP

#include <vulkan/vulkan.hpp>

namespace common {

class VulkanCore {
public:
    virtual ~VulkanCore() = 0;
    //virtual VkPhysicalDevice get_pdev() const = 0;
    virtual VkPhysicalDeviceMemoryProperties get_mem_properties() const = 0;
    virtual VkDevice get_dev() const = 0;
    virtual VkQueue get_queue() const = 0;
    virtual VkCommandBuffer get_transfer_cmdbuf() const = 0;
};

} // namespace common

#endif