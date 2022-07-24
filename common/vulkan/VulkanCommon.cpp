#include <cassert>
#include <cstring>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "VulkanCore.hpp"
#include "VulkanCommon.hpp"

namespace common {

GfxBuffer::~GfxBuffer() {
    vkFreeMemory(core->get_dev(), memory, nullptr);
    vkDestroyBuffer(core->get_dev(), buffer, nullptr);
}

void GfxBuffer::init(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags req_prop) {
    VkBufferCreateInfo bufInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    vkCreateBuffer(core->get_dev(), &bufInfo, nullptr, &buffer);

    vkGetBufferMemoryRequirements(core->get_dev(), buffer, &req);

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = req.size,
        .memoryTypeIndex = findMemoryType(core->get_mem_properties(), req.memoryTypeBits, req_prop),
    };
    vkAllocateMemory(core->get_dev(), &allocInfo, nullptr, &memory);

    vkBindBufferMemory(core->get_dev(), buffer, memory, 0);
}

void GfxBuffer::device_upload(const void *src, VkDeviceSize size) {
    // init staging buffer
    VkBuffer buf_staging;
    VkDeviceMemory bufmem_staging;
    VkBufferCreateInfo bufInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = size;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(core->get_dev(), &bufInfo, nullptr, &buf_staging);

    VkMemoryAllocateInfo allocInfo { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = req.size;
    allocInfo.memoryTypeIndex = findMemoryType(core->get_mem_properties(), req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkAllocateMemory(core->get_dev(), &allocInfo, nullptr, &bufmem_staging);

    vkBindBufferMemory(core->get_dev(), buf_staging, bufmem_staging, 0);

    // update via cpu
    void *buf_ptr;
    vkMapMemory(core->get_dev(), bufmem_staging, 0, size, 0, &buf_ptr);
    memcpy(buf_ptr, src, size);
    vkUnmapMemory(core->get_dev(), bufmem_staging);

    // upload via gpu
    VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(core->get_transfer_cmdbuf(), &cmdbuf_begin_info);
    VkBufferCopy region { 0, 0, bufInfo.size };
    vkCmdCopyBuffer(core->get_transfer_cmdbuf(), buf_staging, buffer, 1, &region);
    vkEndCommandBuffer(core->get_transfer_cmdbuf());

    VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    auto temp = core->get_transfer_cmdbuf(); submitInfo.pCommandBuffers = &temp;

    VkFence fence;
    VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(core->get_dev(), &fence_info, nullptr, &fence);
    vkQueueSubmit(core->get_queue(), 1, &submitInfo, fence);
    vkWaitForFences(core->get_dev(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(core->get_dev(), fence, nullptr);
    vkFreeMemory(core->get_dev(), bufmem_staging, nullptr);
    vkDestroyBuffer(core->get_dev(), buf_staging, nullptr);
    vkResetCommandBuffer(core->get_transfer_cmdbuf(), VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
}

void GfxBuffer::host_update(const void *src, VkDeviceSize size) {
    void *buf_ptr;
    vkMapMemory(core->get_dev(), memory, 0, size, 0, &buf_ptr);
    memcpy(buf_ptr, src, size);
    vkUnmapMemory(core->get_dev(), memory);
}

void GfxBuffer::set_descriptor_info(VkDeviceSize offset, VkDeviceSize size) {
    descriptor_info.buffer = buffer;
    descriptor_info.offset = offset;
    descriptor_info.range = size;
}

GfxImage::~GfxImage() {
    vkDestroyImageView(core->get_dev(), view, nullptr);
    vkFreeMemory(core->get_dev(), memory, nullptr);
    vkDestroyImage(core->get_dev(), image, nullptr);
}

void GfxImage::init(const VkExtent3D ext, VkFormat fmt, VkImageUsageFlags usage, VkMemoryPropertyFlags req_prop, VkImageAspectFlags aspect, uint32_t nof_layer) {
    extent = ext;
    m_aspect_mask = aspect;
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_width = ext.width;
    m_height = ext.height;

    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = fmt,
        .extent = ext,
        .mipLevels = 1,
        .arrayLayers = nof_layer,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    vkCreateImage(core->get_dev(), &imageInfo, nullptr, &image);

    vkGetImageMemoryRequirements(core->get_dev(), image, &req);

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = req.size,
        .memoryTypeIndex = findMemoryType(core->get_mem_properties(), req.memoryTypeBits, req_prop),
    };
    vkAllocateMemory(core->get_dev(), &allocInfo, nullptr, &memory);

    vkBindImageMemory(core->get_dev(), image, memory, 0);

    VkImageViewCreateInfo viewInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = fmt,
        .components = VkComponentMapping { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
        .subresourceRange = VkImageSubresourceRange {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = nof_layer,
        },
    };
    vkCreateImageView(core->get_dev(), &viewInfo, nullptr, &view);
}

void GfxImage::set_descriptor_info(VkSampler sampler, VkImageLayout layout) {
    descriptor_info.sampler = sampler;
    descriptor_info.imageView = view;
    descriptor_info.imageLayout = layout;
}

void GfxImage2D::device_upload(const void *src, VkDeviceSize size, VkImageSubresourceRange & range, VkBufferImageCopy & region) {
    assert(m_layout == VK_IMAGE_LAYOUT_UNDEFINED);

    // first, upload the linear content to a staging buffer
    GfxBuffer *linear_buf = new GfxBuffer(core);
    linear_buf->init(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    linear_buf->host_update(src, size);

    VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(core->get_transfer_cmdbuf(), &beginInfo);

    // second, transition image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier imb {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = range,
    };

    vkCmdPipelineBarrier(core->get_transfer_cmdbuf(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imb);

    // third, blit the buffer into image
    vkCmdCopyBufferToImage(core->get_transfer_cmdbuf(), linear_buf->get_buffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region); // must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL

    vkEndCommandBuffer(core->get_transfer_cmdbuf());

    VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    auto temp = core->get_transfer_cmdbuf(); submitInfo.pCommandBuffers = &temp;

    VkFence fence;
    VkFenceCreateInfo fenceInfo { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(core->get_dev(), &fenceInfo, nullptr, &fence);
    vkQueueSubmit(core->get_queue(), 1, &submitInfo, fence);
    vkWaitForFences(core->get_dev(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(core->get_dev(), fence, nullptr);
    delete linear_buf;
    vkResetCommandBuffer(core->get_transfer_cmdbuf(), VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    m_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    m_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
}

void GfxImage2D::bake(const std::string filename, VkImageUsageFlags usage) {
    int w, h, c;
    uint8_t *ptr = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
    assert(ptr);

    this->init(VkExtent3D { (uint32_t)w, (uint32_t)h, 1 }, VK_FORMAT_R8G8B8A8_SRGB, usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageSubresourceRange range {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    VkBufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = VkImageSubresourceLayers {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = VkOffset3D {},
        .imageExtent = VkExtent3D { (uint32_t)w, (uint32_t)h, 1 },
    };
    // Currently, only support 4 channels format, so that make the larger size than usual to satisify buffer to image blit
    this->device_upload(ptr, sizeof(uint8_t)*w*h*4, range, region);

    stbi_image_free(ptr);
}

void GfxImage2D::transition(VkImageLayout new_layout, VkPipelineStageFlags stage_mask) {
    VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(core->get_transfer_cmdbuf(), &beginInfo);

    VkImageMemoryBarrier imb { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    imb.oldLayout = m_layout;
    imb.newLayout = new_layout;
    imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.image = image;
    imb.subresourceRange = VkImageSubresourceRange {
        .aspectMask = m_aspect_mask,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    if (m_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (m_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    } else if (m_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        {
            // make validation happy
            m_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            imb.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }
        imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else {
        assert(0);
    }
    vkCmdPipelineBarrier(core->get_transfer_cmdbuf(), m_stage_mask, stage_mask, 0, 0, nullptr, 0, nullptr, 1, &imb);

    vkEndCommandBuffer(core->get_transfer_cmdbuf());

    VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    auto temp = core->get_transfer_cmdbuf(); submitInfo.pCommandBuffers = &temp;

    VkFence fence;
    VkFenceCreateInfo fenceInfo { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(core->get_dev(), &fenceInfo, nullptr, &fence);
    vkQueueSubmit(core->get_queue(), 1, &submitInfo, fence);
    vkWaitForFences(core->get_dev(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(core->get_dev(), fence, nullptr);
    vkResetCommandBuffer(core->get_transfer_cmdbuf(), VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    m_layout = new_layout;
    m_stage_mask = stage_mask;
}

uint32_t findMemoryType(VkPhysicalDeviceMemoryProperties mem_properties, uint32_t memoryTypeBit, VkMemoryPropertyFlags request_prop) {
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if (memoryTypeBit & (1 << i)) { // match the desired memory type (a reported memoryTypeBit could match multiples)
            if ((mem_properties.memoryTypes[i].propertyFlags & request_prop) == request_prop) { // match the request mempry properties
                return i;
            }
        }
    }
    assert(0);
    return -1;
}

VkShaderModule loadSPIRV(VkDevice &dev, const std::string str) {
    std::ifstream f(str, std::ios::ate | std::ios::binary);
    size_t size = (size_t)f.tellg();
    std::vector<char> spv(size);
    f.seekg(0);
    f.read(spv.data(), size);
    f.close();

    VkShaderModule shader;
    VkShaderModuleCreateInfo shader_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    shader_info.codeSize = spv.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(spv.data());
    vkCreateShaderModule(dev, &shader_info, nullptr, &shader);

    return shader;
}

void begin_cmdbuf(VkCommandBuffer cmdbuf) {
    VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(cmdbuf, &beginInfo);
}

void submit_and_wait(VkCommandBuffer cmdbuf, VkQueue queue) {
    VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdbuf;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
}

void end_cmdbuf(VkCommandBuffer cmdbuf) {
    vkEndCommandBuffer(cmdbuf);
}

VkPipelineInputAssemblyStateCreateInfo GfxPipelineInputAssemblyState(VkPrimitiveTopology prim) {
    return VkPipelineInputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = prim,
        .primitiveRestartEnable = VK_FALSE, // xxx_list must be false.
    };
}

VkViewport GfxPipelineViewport(int w, int h) {
    return VkViewport {
        .x = 0,
        .y = 0,
        .width = (float)w,
        .height = (float)h,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };
}

VkRect2D GfxPipelineScissor(int w, int h) {
    return VkRect2D {
        .offset = VkOffset2D { 0, 0 },
        .extent = VkExtent2D { (uint32_t)w, (uint32_t)h }
    };
}

VkPipelineViewportStateCreateInfo GfxPipelineViewportState(VkViewport *vp, VkRect2D *scissor) {
    return VkPipelineViewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = vp,
        .scissorCount = 1,
        .pScissors = scissor,
    };
}

VkPipelineRasterizationStateCreateInfo GfxPipelineRasterizationState(VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face) {
    return VkPipelineRasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE, // if the depthClamp device feature is disabled, depthClampEnable must be VK_FALSE
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = polygon_mode,
        .cullMode = cull_mode,
        .frontFace = front_face,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0,
        .depthBiasClamp = 0.0,
        .depthBiasSlopeFactor = 0.0,
        .lineWidth = 1.0,
    };
}

VkPipelineMultisampleStateCreateInfo GfxPipelineMultisampleState(VkSampleCountFlagBits sample) {
    return VkPipelineMultisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = sample,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
}

VkPipelineColorBlendAttachmentState GfxPipelineColorBlendAttachmentState(VkBool32 blend_enable) {
    VkPipelineColorBlendAttachmentState blend_att_state {};
    blend_att_state.blendEnable = blend_enable;
    blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    return blend_att_state;
}

VkPipelineColorBlendStateCreateInfo GfxPipelineBlendState(VkPipelineColorBlendAttachmentState *blend_att_state) {
    // color blend
    // if renderPass is not VK_NULL_HANDLE, the pipeline is being created with fragment output interface state,
    // and subpass uses color attachments, pColorBlendState must be a valid pointer to a valid
    // VkPipelineColorBlendStateCreateInfo structure.
    VkPipelineColorBlendStateCreateInfo blend_state { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    blend_state.attachmentCount = 1;
    blend_state.pAttachments = blend_att_state;
    return blend_state;
}

VkDescriptorSetLayoutBinding GfxDescriptorSetLayoutBinding(uint32_t index, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage) {
    return VkDescriptorSetLayoutBinding {
        .binding = index,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = stage,
    };
}

VkDescriptorSetLayoutCreateInfo GfxDescriptorSetLayoutCreateInfo(std::vector<VkDescriptorSetLayoutBinding> & bindings) {
    return VkDescriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (uint32_t)bindings.size(),
        .pBindings = bindings.data()
    };
}

VkPipelineLayoutCreateInfo GfxPipelineLayoutCreateInfo(VkDescriptorSetLayout *dsl, uint32_t count) {
    return VkPipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = count,
        .pSetLayouts = dsl,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
}

VkDescriptorPoolSize GfxDescriptorPoolSize(VkDescriptorType type, uint32_t count) {
    return VkDescriptorPoolSize {
        .type = type,
        .descriptorCount = count
    };
}

VkDescriptorPoolCreateInfo GfxDescriptorPoolCreateInfo(uint32_t max_set, std::vector<VkDescriptorPoolSize> & pool_size) {
    return VkDescriptorPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = max_set,
        .poolSizeCount = (uint32_t)pool_size.size(),
        .pPoolSizes = pool_size.data()
    };
}

VkDescriptorSetAllocateInfo GfxDescriptorSetAllocateInfo(VkDescriptorPool pool, VkDescriptorSetLayout *dsl, uint32_t count) {
    return VkDescriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = count,
        .pSetLayouts = dsl
    };
}

VkWriteDescriptorSet GfxWriteDescriptorSet(VkDescriptorSet ds, uint32_t index, uint32_t count, VkDescriptorType type, VkDescriptorBufferInfo *buffer_info) {
    return VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = ds,
        .dstBinding = index,
        .dstArrayElement = 0,
        .descriptorCount = count,
        .descriptorType = type,
        .pImageInfo = nullptr,
        .pBufferInfo = buffer_info,
        .pTexelBufferView = nullptr
    };
}

VkWriteDescriptorSet GfxWriteDescriptorSet(VkDescriptorSet ds, uint32_t index, uint32_t count, VkDescriptorType type, VkDescriptorImageInfo *image_info) {
    return VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = ds,
        .dstBinding = index,
        .dstArrayElement = 0,
        .descriptorCount = count,
        .descriptorType = type,
        .pImageInfo = image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };
}

} // namespace common
