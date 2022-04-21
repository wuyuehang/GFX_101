#include <cassert>
#include <cstring>
#include <fstream>
#include <SOIL/SOIL.h>

#include "HelloVulkan.hpp"
#include "VulkanCommon.hpp"

BufferObj::~BufferObj() {
    vkFreeMemory(hello_vulkan->dev, memory, nullptr);
    vkDestroyBuffer(hello_vulkan->dev, buffer, nullptr);
}

void BufferObj::init(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags req_prop) {
    VkBufferCreateInfo buf_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    vkCreateBuffer(hello_vulkan->dev, &buf_info, nullptr, &buffer);

    vkGetBufferMemoryRequirements(hello_vulkan->dev, buffer, &req);

    VkMemoryAllocateInfo alloc_info {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
    .allocationSize = req.size,
    .memoryTypeIndex = findMemoryType(hello_vulkan->mem_properties, req.memoryTypeBits, req_prop),
    };
    vkAllocateMemory(hello_vulkan->dev, &alloc_info, nullptr, &memory);

    vkBindBufferMemory(hello_vulkan->dev, buffer, memory, 0);
}

void BufferObj::upload(const void *src, VkDeviceSize size) {
    // init staging buffer
    VkBuffer buf_staging;
    VkDeviceMemory bufmem_staging;
    VkBufferCreateInfo buf_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buf_info.size = size;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(hello_vulkan->dev, &buf_info, nullptr, &buf_staging);

    VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = findMemoryType(hello_vulkan->mem_properties, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkAllocateMemory(hello_vulkan->dev, &alloc_info, nullptr, &bufmem_staging);

    vkBindBufferMemory(hello_vulkan->dev, buf_staging, bufmem_staging, 0);

    void *buf_ptr;
    vkMapMemory(hello_vulkan->dev, bufmem_staging, 0, size, 0, &buf_ptr);
    memcpy(buf_ptr, src, size);

    // upload via gpu
    VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(hello_vulkan->transfer_cmdbuf, &cmdbuf_begin_info);
    VkBufferCopy region { 0, 0, buf_info.size };
    vkCmdCopyBuffer(hello_vulkan->transfer_cmdbuf, buf_staging, buffer, 1, &region);
    vkEndCommandBuffer(hello_vulkan->transfer_cmdbuf);

    VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(hello_vulkan->transfer_cmdbuf);

    VkFence fence;
    VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(hello_vulkan->dev, &fence_info, nullptr, &fence);
    vkQueueSubmit(hello_vulkan->queue, 1, &submit_info, fence);
    vkWaitForFences(hello_vulkan->dev, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(hello_vulkan->dev, fence, nullptr);
    vkFreeMemory(hello_vulkan->dev, bufmem_staging, nullptr);
    vkDestroyBuffer(hello_vulkan->dev, buf_staging, nullptr);
    vkResetCommandBuffer(hello_vulkan->transfer_cmdbuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
}

void BufferObj::update(const void *src, VkDeviceSize size) {
    void *buf_ptr;
    vkMapMemory(hello_vulkan->dev, memory, 0, size, 0, &buf_ptr);
    memcpy(buf_ptr, src, size);
    vkUnmapMemory(hello_vulkan->dev, memory);
}

ImageObj::~ImageObj() {
    vkDestroyImageView(hello_vulkan->dev, view, nullptr);
    vkFreeMemory(hello_vulkan->dev, memory, nullptr);
    vkDestroyImage(hello_vulkan->dev, image, nullptr);
}

void ImageObj::init(const VkExtent3D ext, VkFormat fmt, VkImageUsageFlags usage, VkMemoryPropertyFlags req_prop, VkImageAspectFlags aspect) {
    extent = ext;
    m_aspect_mask = aspect;
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo image_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = fmt,
        .extent = ext,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    vkCreateImage(hello_vulkan->dev, &image_info, nullptr, &image);

    vkGetImageMemoryRequirements(hello_vulkan->dev, image, &req);

    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = req.size,
        .memoryTypeIndex = findMemoryType(hello_vulkan->mem_properties, req.memoryTypeBits, req_prop),
    };
    vkAllocateMemory(hello_vulkan->dev, &alloc_info, nullptr, &memory);

    vkBindImageMemory(hello_vulkan->dev, image, memory, 0);

    VkImageViewCreateInfo view_info {
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
        .layerCount = 1,},
    };
    vkCreateImageView(hello_vulkan->dev, &view_info, nullptr, &view);
}

void ImageObj::upload(const void *src, VkDeviceSize size, VkImageSubresourceRange &range, VkBufferImageCopy &region) {
    assert(m_layout == VK_IMAGE_LAYOUT_UNDEFINED);

    // first, upload the linear content to a staging buffer
    BufferObj *linear_buf = new BufferObj(hello_vulkan);
    linear_buf->init(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    linear_buf->update(src, size);

    VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(hello_vulkan->transfer_cmdbuf, &cmdbuf_begin_info);

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

    vkCmdPipelineBarrier(hello_vulkan->transfer_cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imb);

    // third, blit the buffer into image
    vkCmdCopyBufferToImage(hello_vulkan->transfer_cmdbuf, linear_buf->get_buffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region); // must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL

    vkEndCommandBuffer(hello_vulkan->transfer_cmdbuf);

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(hello_vulkan->transfer_cmdbuf);

    VkFence fence;
    VkFenceCreateInfo fence_info {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(hello_vulkan->dev, &fence_info, nullptr, &fence);
    vkQueueSubmit(hello_vulkan->queue, 1, &submit_info, fence);
    vkWaitForFences(hello_vulkan->dev, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(hello_vulkan->dev, fence, nullptr);
    delete linear_buf;
    vkResetCommandBuffer(hello_vulkan->transfer_cmdbuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    m_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    m_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
}

void ImageObj::bake(const std::string filename) {
    int w, h, c;
    uint8_t *ptr = SOIL_load_image(filename.c_str(), &w, &h, &c, SOIL_LOAD_RGBA);
    assert(ptr);

    this->init(VkExtent3D { (uint32_t)w, (uint32_t)h, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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
    this->upload(ptr, sizeof(uint8_t)*w*h*4, range, region);

    SOIL_free_image_data(ptr);
}

void ImageObj::transition(VkImageLayout new_layout, VkPipelineStageFlags stage_mask) {
    VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(hello_vulkan->transfer_cmdbuf, &cmdbuf_begin_info);

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
    } else {
        assert(0);
    }
    vkCmdPipelineBarrier(hello_vulkan->transfer_cmdbuf, m_stage_mask, stage_mask, 0, 0, nullptr, 0, nullptr, 1, &imb);

    vkEndCommandBuffer(hello_vulkan->transfer_cmdbuf);

    VkSubmitInfo submit_info {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(hello_vulkan->transfer_cmdbuf);

    VkFence fence;
    VkFenceCreateInfo fence_info {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(hello_vulkan->dev, &fence_info, nullptr, &fence);
    vkQueueSubmit(hello_vulkan->queue, 1, &submit_info, fence);
    vkWaitForFences(hello_vulkan->dev, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(hello_vulkan->dev, fence, nullptr);
    vkResetCommandBuffer(hello_vulkan->transfer_cmdbuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

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
