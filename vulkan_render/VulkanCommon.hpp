#ifndef VULKAN_COMMON_HPP
#define VULKAN_COMMON_HPP

#include <vulkan/vulkan.h>
#include <cassert>
#include <fstream>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct SCENE {
    glm::vec3 light_loc; // view space location of light, f.inst point light
    float roughness; // specular
};

class HelloVulkan;

class BufferObj {
public:
    BufferObj() = delete;
    BufferObj(const BufferObj &) = delete;
    BufferObj(const HelloVulkan *app) : hello_vulkan(app) { assert(app != nullptr); }
    ~BufferObj();
    void init(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags req_prop);
    void upload(const void *src, VkDeviceSize size);
    const VkBuffer& get_buffer() const { return buffer; }
    const VkDeviceMemory& get_memory() const { return memory; }
private:
    friend class ImageObj;
    void update(const void *src, VkDeviceSize size);
    const HelloVulkan *hello_vulkan;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkBufferView view;
    VkMemoryRequirements req;
};

struct ImageObj {
public:
    ImageObj() = delete;
    ImageObj(const ImageObj &) = delete;
    ImageObj(HelloVulkan *app) : hello_vulkan(app) { assert(app != nullptr); }
    void init(const VkExtent3D, VkFormat, VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags);
    void upload(const void *, VkDeviceSize, VkImageSubresourceRange &, VkBufferImageCopy &);
    void bake(const std::string);
    void transition(VkImageLayout, VkPipelineStageFlags);
    const VkImageView& get_image_view() const { return view; }
    ~ImageObj();
private:
    const HelloVulkan *hello_vulkan;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkMemoryRequirements req;
    VkExtent3D extent;
    VkImageLayout m_layout;
    VkImageAspectFlags m_aspect_mask;
    VkPipelineStageFlags m_stage_mask;
};

uint32_t findMemoryType(VkPhysicalDeviceMemoryProperties mem_properties, uint32_t memoryTypeBit, VkMemoryPropertyFlags request_prop);

VkShaderModule loadSPIRV(VkDevice &dev, const std::string str);

#endif