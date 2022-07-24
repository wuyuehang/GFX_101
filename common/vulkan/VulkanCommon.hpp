#ifndef VULKAN_COMMON_HPP
#define VULKAN_COMMON_HPP

#include <vulkan/vulkan.h>
#include <cassert>
#include <fstream>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace common {

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

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
    void set_descriptor_info(VkDeviceSize offset, VkDeviceSize size);
    const VkBuffer & get_buffer() const { return buffer; }
    const VkDeviceMemory & get_memory() const { return memory; }
    VkDescriptorBufferInfo & get_descriptor_info() { return descriptor_info; }
private:
    friend class GfxImage;
    const VulkanCore *core;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkBufferView view;
    VkMemoryRequirements req;
    VkDescriptorBufferInfo descriptor_info;
};

class GfxImage {
public:
    GfxImage() = delete;
    GfxImage &operator=(const GfxImage &) = delete;
    GfxImage(const GfxImage &) = delete;
    GfxImage(VulkanCore *app) : core(app) { assert(app != nullptr); }
    void init(const VkExtent3D ext, VkFormat fmt, VkImageUsageFlags usage, VkMemoryPropertyFlags req_prop, VkImageAspectFlags aspect, uint32_t nof_layer = 1);
    void set_descriptor_info(VkSampler sampler, VkImageLayout layout);
    const VkImageView& get_image_view() const { return view; }
    const VkImage& get_image() const { return image; }
    const uint32_t get_width() const { return m_width; }
    const uint32_t get_height() const { return m_height; }
    VkDescriptorImageInfo & get_descriptor_info() { return descriptor_info; }
    ~GfxImage();
protected:
    const VulkanCore *core;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkMemoryRequirements req;
    VkExtent3D extent;
    VkImageLayout m_layout;
    VkImageAspectFlags m_aspect_mask;
    VkPipelineStageFlags m_stage_mask;
    uint32_t m_width;
    uint32_t m_height;
    VkDescriptorImageInfo descriptor_info;
};

class GfxImage2D : public GfxImage {
public:
    GfxImage2D() = delete;
    GfxImage2D &operator=(const GfxImage2D &) = delete;
    GfxImage2D(const GfxImage2D &) = delete;
    GfxImage2D(VulkanCore *app) : GfxImage(app) { assert(app != nullptr); }
    void device_upload(const void *, VkDeviceSize, VkImageSubresourceRange &, VkBufferImageCopy &);
    void bake(const std::string, VkImageUsageFlags usage);
    void transition(VkImageLayout, VkPipelineStageFlags);
    ~GfxImage2D() {}
};

class GfxImage2DArray : public GfxImage {
public:
    GfxImage2DArray() = delete;
    GfxImage2DArray &operator=(const GfxImage2DArray &) = delete;
    GfxImage2DArray(const GfxImage2DArray &) = delete;
    GfxImage2DArray(VulkanCore *app) : GfxImage(app) { assert(app != nullptr); }
    ~GfxImage2DArray() {}
};

uint32_t findMemoryType(VkPhysicalDeviceMemoryProperties mem_properties, uint32_t memoryTypeBit, VkMemoryPropertyFlags request_prop);
VkShaderModule loadSPIRV(VkDevice &dev, const std::string str);

void begin_cmdbuf(VkCommandBuffer cmdbuf);
void submit_and_wait(VkCommandBuffer cmdbuf, VkQueue queue);
void end_cmdbuf(VkCommandBuffer cmdbuf);

VkPipelineInputAssemblyStateCreateInfo GfxPipelineInputAssemblyState(VkPrimitiveTopology prim);
VkViewport GfxPipelineViewport(int w, int h);
VkRect2D GfxPipelineScissor(int w, int h);
VkPipelineViewportStateCreateInfo GfxPipelineViewportState(VkViewport *vp, VkRect2D *scissor);
VkPipelineRasterizationStateCreateInfo GfxPipelineRasterizationState(VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face);
VkPipelineMultisampleStateCreateInfo GfxPipelineMultisampleState(VkSampleCountFlagBits sample);
VkPipelineColorBlendAttachmentState GfxPipelineColorBlendAttachmentState(VkBool32 blend_enable=VK_FALSE);
VkPipelineColorBlendStateCreateInfo GfxPipelineBlendState(VkPipelineColorBlendAttachmentState *blend_att_state);

VkDescriptorSetLayoutBinding GfxDescriptorSetLayoutBinding(uint32_t index, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage);
VkDescriptorSetLayoutCreateInfo GfxDescriptorSetLayoutCreateInfo(std::vector<VkDescriptorSetLayoutBinding> & bindings);
VkPipelineLayoutCreateInfo GfxPipelineLayoutCreateInfo(VkDescriptorSetLayout *dsl, uint32_t count);
VkDescriptorPoolSize GfxDescriptorPoolSize(VkDescriptorType type, uint32_t count);
VkDescriptorPoolCreateInfo GfxDescriptorPoolCreateInfo(uint32_t max_set, std::vector<VkDescriptorPoolSize> & pool_size);
VkDescriptorSetAllocateInfo GfxDescriptorSetAllocateInfo(VkDescriptorPool pool, VkDescriptorSetLayout *dsl, uint32_t count);
VkWriteDescriptorSet GfxWriteDescriptorSet(VkDescriptorSet ds, uint32_t index, uint32_t count, VkDescriptorType type, VkDescriptorBufferInfo *buffer_info);
VkWriteDescriptorSet GfxWriteDescriptorSet(VkDescriptorSet ds, uint32_t index, uint32_t count, VkDescriptorType type, VkDescriptorImageInfo *image_info);

} // namespace common

#endif
