#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "VulkanCommon.hpp"
#include "SkeletonVulkan.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace common {

class TestImage : public SkeletonVulkan {
public:
    TestImage() : SkeletonVulkan(VK_API_VERSION_1_0) {
        SkeletonVulkan::init();
        // 1. implicit copy buffer --> image
        auto I0 = new GfxImage2D(this);
        I0->bake("../resource/buu.png", VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        I0->transition(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // 2. explicit blit image --> image
        auto I1 = new GfxImage2D(this);
        I1->init(VkExtent3D { I0->get_width(), I0->get_height(), 1 }, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        I1->transition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);

        begin_cmdbuf(transfer_cmdbuf);
        VkImageBlit imageBlit {
            .srcSubresource = VkImageSubresourceLayers {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets = {
                VkOffset3D {},
                VkOffset3D { (int32_t)I0->get_width(), (int32_t)I0->get_height(), 1 }
            },
            .dstSubresource = VkImageSubresourceLayers {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = {
                VkOffset3D {},
                VkOffset3D { (int32_t)I0->get_width(), (int32_t)I0->get_height(), 1 }
            },
        };
        vkCmdBlitImage(transfer_cmdbuf, I0->get_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            I1->get_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imageBlit, VK_FILTER_NEAREST);

        end_cmdbuf(transfer_cmdbuf);
        submit_and_wait(transfer_cmdbuf, queue);

        // 3. explicit copy image --> image
        I1->transition(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);

        auto I2 = new GfxImage2D(this);
        I2->init(VkExtent3D { I1->get_width(), I1->get_height(), 1 }, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        I2->transition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);

        begin_cmdbuf(transfer_cmdbuf);
        VkImageCopy imageCopy {
            .srcSubresource = VkImageSubresourceLayers {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffset = VkOffset3D {},
            .dstSubresource = VkImageSubresourceLayers {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffset = VkOffset3D {},
            .extent = VkExtent3D { I0->get_width(), I0->get_height(), 1 },
        };
        vkCmdCopyImage(transfer_cmdbuf, I1->get_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            I2->get_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imageCopy);

        end_cmdbuf(transfer_cmdbuf);
        submit_and_wait(transfer_cmdbuf, queue);

        // 4. explicit copy image --> buffer
        I2->transition(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);

        auto B0 = new GfxBuffer(this);
        B0->init(I2->get_width()*I2->get_height()*4, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        begin_cmdbuf(transfer_cmdbuf);
        VkBufferImageCopy bufferImageCopy {
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
            .imageExtent = VkExtent3D { I2->get_width(), I2->get_height(), 1 },
        };
        vkCmdCopyImageToBuffer(transfer_cmdbuf, I2->get_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, B0->get_buffer(), 1, &bufferImageCopy);
        end_cmdbuf(transfer_cmdbuf);
        submit_and_wait(transfer_cmdbuf, queue);

        {
            void *buf_ptr;
            vkMapMemory(dev, B0->get_memory(), 0, I2->get_width()*I2->get_height()*4, 0, &buf_ptr);
            stbi_write_png("result.png", I2->get_width(), I2->get_height(), 4, buf_ptr, I2->get_width() * 4);
            vkUnmapMemory(dev, B0->get_memory());
        }
        delete I0; delete I1; delete I2; delete B0;
    }
    ~TestImage() {}
};

}
int main() {
    common::TestImage T;
    return 0;
}
