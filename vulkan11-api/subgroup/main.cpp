#include <vulkan/vulkan.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "VulkanCommon.hpp"
#include "SkeletonComputeVulkan.hpp"

#define ELE_SZIE sizeof(int)
#define ELE_NUM 32
#define SSBO_SIZE (ELE_NUM * ELE_SZIE)

namespace common {

class SubGroup : public SkeletonComputeVulkan {
public:
    SubGroup() : SkeletonComputeVulkan(VK_API_VERSION_1_2) {
        m_device_extension.push_back("VK_EXT_subgroup_size_control");
        m_device_extension.push_back("VK_EXT_shader_subgroup_vote");
        m_device_extension.push_back("VK_EXT_shader_subgroup_ballot");

        VkPhysicalDeviceSubgroupSizeControlFeaturesEXT ssc;
        ssc.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT;
        ssc.pNext = nullptr;

        ssc.subgroupSizeControl = VK_FALSE;
        ssc.computeFullSubgroups = VK_TRUE;
        m_device_info_next = &ssc;
        SkeletonComputeVulkan::init();

        // Query subgroup feature
        VkPhysicalDeviceFeatures2 feature2 {};
        feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        feature2.pNext = &ssc;
        vkGetPhysicalDeviceFeatures2(pdev[0], &feature2);
        if (ssc.subgroupSizeControl) {
            std::cout << "Implementation supports controlling shader subgroup sizes" << std::endl;
        }
        if (ssc.computeFullSubgroups) {
            std::cout << "Implementation supports requiring full subgroups in compute shaders" << std::endl;
        }

        // Query subgroup properties
        VkPhysicalDeviceSubgroupProperties subgroup_prop { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };
        VkPhysicalDeviceProperties2 prop2 { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        prop2.pNext = &subgroup_prop;
        vkGetPhysicalDeviceProperties2(pdev[0], &prop2);
        std::cout << "Max Workgroup Count.x    : " << prop2.properties.limits.maxComputeWorkGroupCount[0] << std::endl;
        std::cout << "Max Workgroup Count.y    : " << prop2.properties.limits.maxComputeWorkGroupCount[1] << std::endl;
        std::cout << "Max Workgroup Count.z    : " << prop2.properties.limits.maxComputeWorkGroupCount[2] << std::endl;
        std::cout << "Max Workgroup Size.x     : " << prop2.properties.limits.maxComputeWorkGroupSize[0] << std::endl;
        std::cout << "Max Workgroup Size.y     : " << prop2.properties.limits.maxComputeWorkGroupSize[1] << std::endl;
        std::cout << "Max Workgroup Size.z     : " << prop2.properties.limits.maxComputeWorkGroupSize[2] << std::endl;
        std::cout << "Max Workgroup Invocation : " << prop2.properties.limits.maxComputeWorkGroupInvocations << std::endl;

        std::cout << "Subgroup Size (Warp)     : " << subgroup_prop.subgroupSize << std::endl;
        #define QUERY_SUBGROUP_FEATURE(FEATURE) \
        do { if (subgroup_prop.supportedOperations & FEATURE) std::cout << #FEATURE << std::endl; } while (0);

        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_BASIC_BIT)
        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_VOTE_BIT)
        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_ARITHMETIC_BIT)
        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_BALLOT_BIT)
        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_SHUFFLE_BIT)
        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT)
        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_CLUSTERED_BIT)
        QUERY_SUBGROUP_FEATURE(VK_SUBGROUP_FEATURE_QUAD_BIT)

        #define QUERY_SUBGROUP_STAGE(STAGE) \
        do { if (subgroup_prop.supportedStages & STAGE) std::cout << #STAGE << std::endl; } while (0);
        QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_VERTEX_BIT)
        QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
        QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
        QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_GEOMETRY_BIT)
        QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_FRAGMENT_BIT)
        QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_COMPUTE_BIT)
        //QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_RAYGEN_BIT_KHR)
        //QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
        //QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
        //QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_MISS_BIT_KHR)
        //QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_INTERSECTION_BIT_KHR)
        //QUERY_SUBGROUP_STAGE(VK_SHADER_STAGE_CALLABLE_BIT_KHR)
        CreateShaderStorageBuffer();
        CreateDescriptor();
    }
    ~SubGroup() {
        vkDeviceWaitIdle(dev);
        delete ssbo; delete result;
        for (auto pipeline : pipelines) {
            vkDestroyPipeline(dev, pipeline, nullptr);
        }
        vkDestroyPipelineLayout(dev, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
        vkFreeDescriptorSets(dev, ds_pool, 1, &ds);
        vkDestroyDescriptorPool(dev, ds_pool, nullptr);
    }
    void CreateShaderStorageBuffer() {
        ssbo = new GfxBuffer(this);
        ssbo->init(SSBO_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        ssbo->set_descriptor_info(0, SSBO_SIZE);
        result = new GfxBuffer(this);
        result->init(SSBO_SIZE, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
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
    VkPipeline CreatePipeline(std::string spv) {
        VkPipeline pipeline;
        auto comp = loadSPIRV(dev, spv);

        VkPipelineShaderStageCreateInfo comp_stage_info { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        comp_stage_info.flags = VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT_EXT; // force populate full subgroup size
        comp_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        comp_stage_info.module = comp;
        comp_stage_info.pName = "main";

        VkComputePipelineCreateInfo pipe_info { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        pipe_info.stage = comp_stage_info;
        pipe_info.layout = pipeline_layout;
        vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipe_info, nullptr, &pipeline);

        vkDestroyShaderModule(dev, comp, nullptr);
        return pipeline;
    }
    void Run() {
        {
            VkPipeline p = CreatePipeline("gl_SubgroupInvocationID.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == i);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupElect.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                if (i == 0) {
                    assert(*(static_cast<int *>(buf_ptr) + i) == i);
                } else {
                    assert(*(static_cast<int *>(buf_ptr) + i) == 66666666);
                }
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        // VK_EXT_shader_subgroup_vote
        {
            VkPipeline p = CreatePipeline("subgroupAll.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == i);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupAny.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, 4, 0); // only first element is filled with 0
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == i);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupAllEqual.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == i);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        // GL_KHR_shader_subgroup_arithmetic
        {
            VkPipeline p = CreatePipeline("subgroupAdd.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == (0 + 31) * 32 / 2);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupMax.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == 31);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupInclusiveAdd.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == (0 + i) * (i + 1) /2);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupExclusiveAdd.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                if (i == 0) {
                    assert(*(static_cast<int *>(buf_ptr) + i) == 0);
                } else {
                    assert(*(static_cast<int *>(buf_ptr) + i) == (0 + i - 1) * (i - 1 + 1) /2);
                }
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        // VK_EXT_shader_subgroup_ballot
        {
            VkPipeline p = CreatePipeline("subgroupBroadcast.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == 31);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupBroadcastFirst.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == 0);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupBallot.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << std::hex << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == 0x55555555);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupBallotFindLSB.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << std::hex << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == 0);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupBallotFindMSB.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << std::hex << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == 30);
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupShuffle.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == (31 - i));
            }
            vkUnmapMemory(dev, result->get_memory());
        }
        {
            VkPipeline p = CreatePipeline("subgroupClusteredAdd.comp.spv"); pipelines.push_back(p);
            begin_cmdbuf(transfer_cmdbuf);
            vkCmdBindPipeline(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(transfer_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(transfer_cmdbuf, ssbo->get_buffer(), 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(transfer_cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(transfer_cmdbuf, ssbo->get_buffer(), result->get_buffer(), 1, &region);
            end_cmdbuf(transfer_cmdbuf);
            submit_and_wait(transfer_cmdbuf, queue);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, result->get_memory(), 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                //std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
                assert(*(static_cast<int *>(buf_ptr) + i) == (6 + (i / 4) * 16));
            }
            vkUnmapMemory(dev, result->get_memory());
        }
    }
private:
    VkDescriptorSetLayout dsl;
    VkDescriptorPool ds_pool;
    VkDescriptorSet ds;
    VkPipelineLayout pipeline_layout;
    std::vector<VkPipeline> pipelines;
    GfxBuffer *ssbo;
    GfxBuffer *result;
};
}

int main(int argc, char **argv) {
    common::SubGroup demo;
    demo.Run();
    return 0;
}
