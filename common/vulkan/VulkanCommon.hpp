#ifndef VULKAN_COMMON_HPP
#define VULKAN_COMMON_HPP

#include <vulkan/vulkan.h>
#include <cassert>
#include <fstream>
#include <string>
#include <vector>

namespace common {

class VulkanCore;

class GfxBuffer {
public:
    GfxBuffer() = delete;
    GfxBuffer &operator=(const GfxBuffer &) = delete;
    GfxBuffer(const GfxBuffer &) = delete;
    GfxBuffer(const VulkanCore *app) : core(app) { assert(app != nullptr); }
    ~GfxBuffer();
    void init(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags req_prop);
    void device_upload(const void *src, VkDeviceSize size);
    void host_update(const void *src, VkDeviceSize size);
    const VkBuffer & get_buffer() const { return buffer; }
    const VkDeviceMemory & get_memory() const { return memory; }
private:
    friend class GfxImage;
    const VulkanCore *core;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkBufferView view;
    VkMemoryRequirements req;
};

#if 0
class GfxImage {
public:
    GfxImage() = delete;
    GfxImage(const GfxImage &) = delete;
    GfxImage(VulkanCore *app) : core(app) { assert(app != nullptr); }
    void init(const VkExtent3D, VkFormat, VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags);
    void upload(const void *, VkDeviceSize, VkImageSubresourceRange &, VkBufferImageCopy &);
    void bake(const std::string);
    void transition(VkImageLayout, VkPipelineStageFlags);
    const VkImageView& get_image_view() const { return view; }
    ~GfxImage();
private:
    const VulkanCore *core;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkMemoryRequirements req;
    VkExtent3D extent;
    VkImageLayout m_layout;
    VkImageAspectFlags m_aspect_mask;
    VkPipelineStageFlags m_stage_mask;
};
#endif

#if 0
class Image2DArray {
public:
    Image2DArray() = delete;
    Image2DArray(const Image2DArray &) = delete;
    Image2DArray(VulkanCore *app) : core(app) { assert(app != nullptr); }
    void init(const VkExtent3D, VkFormat, VkImageUsageFlags, VkMemoryPropertyFlags, VkImageAspectFlags, uint32_t);
    //void upload(const void *, VkDeviceSize, VkImageSubresourceRange &, VkBufferImageCopy &);
    //void bake(const std::string);
    //void transition(VkImageLayout, VkPipelineStageFlags);
    const VkImageView& get_image_view() const { return view; }
    ~Image2DArray();
private:
    const VulkanCore *core;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkMemoryRequirements req;
    VkExtent3D extent;
    VkImageLayout m_layout;
    VkImageAspectFlags m_aspect_mask;
    VkPipelineStageFlags m_stage_mask;
};
#endif

uint32_t findMemoryType(VkPhysicalDeviceMemoryProperties mem_properties, uint32_t memoryTypeBit, VkMemoryPropertyFlags request_prop);
//VkShaderModule loadSPIRV(VkDevice &dev, const std::string str);

} // namespace common
#endif
