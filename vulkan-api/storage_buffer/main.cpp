#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "VulkanCommon.hpp"
#include "SkeletonVulkan.hpp"

#define BUFFER_COPY_SIZE 64

namespace common {

class TestStorageBuffer : public SkeletonVulkan {
public:
    TestStorageBuffer() : SkeletonVulkan(VK_API_VERSION_1_0) {
        CreateStorageBuffer();
        CreateDescriptor();
        CreatePipeline();
        Run();
    }
    ~TestStorageBuffer() {
        vkDeviceWaitIdle(dev);
        delete ssbo; delete result_buf;
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
        vkFreeDescriptorSets(dev, ds_pool, 1, &ds);
        vkDestroyDescriptorPool(dev, ds_pool, nullptr);
    }
    void CreateStorageBuffer() {
        ssbo = new GfxBuffer(this);
        ssbo->init(sizeof(uint32_t)*BUFFER_COPY_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        ssbo->set_descriptor_info(0, sizeof(uint32_t)*BUFFER_COPY_SIZE);
        result_buf = new GfxBuffer(this);
        result_buf->init(sizeof(uint32_t)*BUFFER_COPY_SIZE, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    void CreateDescriptor() {
        // descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            GfxDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
        };
        VkDescriptorSetLayoutCreateInfo dsl_info = GfxDescriptorSetLayoutCreateInfo(bindings);
        vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl);

        // pipeline layout
        VkPipelineLayoutCreateInfo pipeline_layout_info = GfxPipelineLayoutCreateInfo(&dsl, 1);
        vkCreatePipelineLayout(dev, &pipeline_layout_info, nullptr, &pipeline_layout);

        // descriptor pool
        std::vector<VkDescriptorPoolSize> pool_size = {
            GfxDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
        };
        VkDescriptorPoolCreateInfo pool_info = GfxDescriptorPoolCreateInfo(1, pool_size);
        vkCreateDescriptorPool(dev, &pool_info, nullptr, &ds_pool);

        // descriptor
        VkDescriptorSetAllocateInfo alloc_info = GfxDescriptorSetAllocateInfo(ds_pool, &dsl, 1);
        vkAllocateDescriptorSets(dev, &alloc_info, &ds);
        std::vector<VkWriteDescriptorSet> w = {
            GfxWriteDescriptorSet(ds, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &ssbo->get_descriptor_info())
        };
        vkUpdateDescriptorSets(dev, w.size(), w.data(), 0, nullptr);
    }
    void Run() {
        begin_cmdbuf(transfer_cmdbuf);
        vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
        vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (64x1x1)
        // copy result back
        VkBufferCopy bufCopy {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = sizeof(uint32_t)*BUFFER_COPY_SIZE
        };
        vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result_buf->get_buffer(), 1, &bufCopy);
        end_cmdbuf(transfer_cmdbuf);
        submit_and_wait(transfer_cmdbuf, queue);
        void *ptr;
        vkMapMemory(dev, result_buf->get_memory(), 0, sizeof(uint32_t)*BUFFER_COPY_SIZE, 0, &ptr);
        for (int i = 0; i < BUFFER_COPY_SIZE; i++) {
            assert(*(static_cast<int32_t *>(ptr) + i) == 0x01020304);
        }
        vkUnmapMemory(dev, result_buf->get_memory());
    }
    void CreatePipeline() {
        auto comp = loadSPIRV(dev, "ssbo.comp.spv");
        VkPipelineShaderStageCreateInfo stage_info { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage_info.module = comp;
        stage_info.pName = "main";

        VkComputePipelineCreateInfo compute_pipeline_info {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = stage_info,
            .layout = pipeline_layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0
        };
        vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &compute_pipeline_info, nullptr, &pipeline);
        vkDestroyShaderModule(dev, comp, nullptr);
    }
    VkDescriptorPool ds_pool;
    VkDescriptorSet ds;
    VkDescriptorSetLayout dsl;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    GfxBuffer *ssbo;
    GfxBuffer *result_buf;
};

}
int main() {
    common::TestStorageBuffer T;
    return 0;
}