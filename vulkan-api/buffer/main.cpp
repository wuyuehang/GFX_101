#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "VulkanCommon.hpp"
#include "SkeletonVulkan.hpp"

namespace common {

class TestBuffer : public SkeletonVulkan {
public:
    TestBuffer() : SkeletonVulkan(VK_API_VERSION_1_0) {
        constexpr uint32_t nbr = 256;
        constexpr uint32_t SZ = nbr * sizeof(uint32_t);
        {
            auto buf = new GfxBuffer(this);
            buf->init(SZ, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vkBeginCommandBuffer(transfer_cmdbuf, &beginInfo);
            vkCmdFillBuffer(transfer_cmdbuf, buf->get_buffer(), 0, SZ, 0x12345678);
            vkEndCommandBuffer(transfer_cmdbuf);

            VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &transfer_cmdbuf;
            vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);

            void *ptr;
            vkMapMemory(dev, buf->get_memory(), 0, SZ, 0, &ptr);
            for (auto i = 0; i < nbr; i++) {
                assert(0x12345678 == *(static_cast<uint32_t *>(ptr) + i));
            }
            vkUnmapMemory(dev, buf->get_memory());
            delete buf;
        }
        {
            auto data = new uint32_t [nbr];
            for (auto i = 0; i < nbr; i++) {
                data[i] = 0x12345678;
            }
            auto buf = new GfxBuffer(this);
            buf->init(SZ, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vkBeginCommandBuffer(transfer_cmdbuf, &beginInfo);
            vkCmdUpdateBuffer(transfer_cmdbuf, buf->get_buffer(), 0, SZ, data);
            vkEndCommandBuffer(transfer_cmdbuf);

            VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &transfer_cmdbuf;
            vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);

            void *ptr;
            vkMapMemory(dev, buf->get_memory(), 0, SZ, 0, &ptr);
            for (auto i = 0; i < nbr; i++) {
                assert(0x12345678 == *(static_cast<uint32_t *>(ptr) + i));
            }
            vkUnmapMemory(dev, buf->get_memory());
            delete buf;
            delete [] data;
        }
        {
            auto srcbuf = new GfxBuffer(this);
            srcbuf->init(SZ, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            auto dstbuf = new GfxBuffer(this);
            dstbuf->init(SZ, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vkBeginCommandBuffer(transfer_cmdbuf, &beginInfo);
            vkCmdFillBuffer(transfer_cmdbuf, srcbuf->get_buffer(), 0, SZ, 0x12345678);

            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SZ,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, srcbuf->get_buffer(), dstbuf->get_buffer(), 1, &region);
            vkEndCommandBuffer(transfer_cmdbuf);

            VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &transfer_cmdbuf;
            vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);

            void *ptr;
            vkMapMemory(dev, dstbuf->get_memory(), 0, SZ, 0, &ptr);
            for (auto i = 0; i < nbr; i++) {
                assert(0x12345678 == *(static_cast<uint32_t *>(ptr) + i));
            }
            vkUnmapMemory(dev, dstbuf->get_memory());
            delete srcbuf; delete dstbuf;
        }
    }
    ~TestBuffer() {}
};

}
int main() {
    common::TestBuffer T;
    return 0;
}