#include <vulkan/vulkan.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define ELE_SZIE sizeof(int)
#define ELE_NUM 32
#define SSBO_SIZE (ELE_NUM * ELE_SZIE)

class SubGroup {
public:
    SubGroup() {
        CreateInstance();
        CreateDevice();
        QuerySubgroupProperties();
        CreateCommandBuffer();
        CreateShaderStorageBuffer();
        BakeComputeDescriptorSet();
    }
    ~SubGroup() {
        vkDeviceWaitIdle(dev);
        for (auto pipeline : pipelines) {
            vkDestroyPipeline(dev, pipeline, nullptr);
        }
        vkDestroyPipelineLayout(dev, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
        vkFreeDescriptorSets(dev, ds_pool, 1, &ds);
        vkDestroyDescriptorPool(dev, ds_pool, nullptr);
        vkFreeMemory(dev, dstbuf_mem, nullptr);
        vkFreeMemory(dev, resbuf_mem, nullptr);
        vkDestroyBuffer(dev, dstbuf, nullptr);
        vkDestroyBuffer(dev, resbuf, nullptr);
        vkFreeCommandBuffers(dev, cmdpool, 1, &cmdbuf);
        vkDestroyCommandPool(dev, cmdpool, nullptr);
        vkDestroyDevice(dev, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
    std::vector<char> loadSPIRV(const std::string str) {
        std::ifstream f(str, std::ios::ate | std::ios::binary);
        size_t size = (size_t)f.tellg();
        std::vector<char> buf(size);
        f.seekg(0);
        f.read(buf.data(), size);
        f.close();
        return buf;
    }
    void CreateInstance() {
        // dynamically fetch vulkan function by dlopen libvulkan.
        // no need to include vulkan header, otherwise it will alias
        // #define vk_global_func( fun ) fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun)
        // vk_global_func( vkCreateInstance );
        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_info.pApplicationInfo = &app_info;
        vkCreateInstance(&instance_info, nullptr, &instance);
    }
    void CreateDevice() {
        uint32_t pdev_count;
        vkEnumeratePhysicalDevices(instance, &pdev_count, nullptr);
        std::vector<VkPhysicalDevice> physical_dev(pdev_count);
        vkEnumeratePhysicalDevices(instance, &pdev_count, physical_dev.data());
        pdev = physical_dev[0];

#if 0
        {
            uint32_t dev_ext_nbr;
            vkEnumerateDeviceExtensionProperties(pdev, nullptr, &dev_ext_nbr, nullptr);
            std::vector<VkExtensionProperties> dev_ext(dev_ext_nbr);
            vkEnumerateDeviceExtensionProperties(pdev, nullptr, &dev_ext_nbr, dev_ext.data());
            for (auto & it : dev_ext) {
                std::cout << "device extension name: " << it.extensionName << std::endl;
            }
        }
#endif
        uint32_t queue_prop_count;
        q_family_index = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queue_prop_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_props(queue_prop_count);
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queue_prop_count, queue_props.data());
        for (auto& it : queue_props) {
            if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT && it.queueFlags & VK_QUEUE_COMPUTE_BIT && it.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                break;
            }
            q_family_index++;
        }

        VkDeviceQueueCreateInfo queue_info { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        float q_prio = 1.0;
        queue_info.queueFamilyIndex = q_family_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &q_prio;

        VkDeviceCreateInfo dev_info { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        dev_info.queueCreateInfoCount = 1;
        dev_info.pQueueCreateInfos = &queue_info;

        std::vector<const char *> dev_ext;
        //dev_ext.push_back("VK_EXT_shader_subgroup_ballot");
        //dev_ext.push_back("VK_EXT_shader_subgroup_vote");
        dev_ext.push_back("VK_EXT_subgroup_size_control");
        dev_info.enabledExtensionCount = dev_ext.size();
        dev_info.ppEnabledExtensionNames = dev_ext.data();
        VkPhysicalDeviceSubgroupSizeControlFeaturesEXT ssc;
        ssc.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT;
        ssc.pNext = nullptr;
        ssc.subgroupSizeControl = VK_FALSE;
        ssc.computeFullSubgroups = VK_TRUE;
#if 1
        dev_info.pNext = &ssc;
#endif

        vkCreateDevice(pdev, &dev_info, nullptr, &dev);
        //VkBool32 present_supported = VK_FALSE;
        //vkGetPhysicalDeviceSurfaceSupportKHR(pdev, q_family_index, surface, &present_supported);
        //assert(VK_TRUE == present_supported); // assume the render queue is the same as present queue
        vkGetDeviceQueue(dev, q_family_index, 0, &queue); // we only create 1 queue from the family queues, its index is 0

        vkGetPhysicalDeviceMemoryProperties(pdev, &mem_properties);
    }
    void QuerySubgroupProperties() {
        VkPhysicalDeviceSubgroupProperties subgroup_prop { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };
        VkPhysicalDeviceProperties2 prop2 { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        prop2.pNext = &subgroup_prop;
        vkGetPhysicalDeviceProperties2(pdev, &prop2);
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
    }
    void CreateCommandBuffer() {
        VkCommandPoolCreateInfo commandpool_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        commandpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow individual commandbuffer reset
        commandpool_info.queueFamilyIndex = q_family_index;
        vkCreateCommandPool(dev, &commandpool_info, nullptr, &cmdpool);

        {
            VkCommandBufferAllocateInfo cmdbuf_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            cmdbuf_info.commandPool = cmdpool;
            cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdbuf_info.commandBufferCount = 1;
            vkAllocateCommandBuffers(dev, &cmdbuf_info, &cmdbuf);
        }
    }
    void CreateShaderStorageBuffer() {
        // initialize dst storage buffer
        VkBufferCreateInfo buf_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buf_info.size = SSBO_SIZE;
        buf_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(dev, &buf_info, nullptr, &dstbuf);

        VkMemoryRequirements req {};
        vkGetBufferMemoryRequirements(dev, dstbuf, &req);

        VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(dev, &alloc_info, nullptr, &dstbuf_mem);

        vkBindBufferMemory(dev, dstbuf, dstbuf_mem, 0);

        {
            // alloc a linear buffer for verifcation
            VkBufferCreateInfo buf_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            buf_info.size = SSBO_SIZE;
            buf_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(dev, &buf_info, nullptr, &resbuf);

            VkMemoryRequirements req {};
            vkGetBufferMemoryRequirements(dev, resbuf, &req);

            VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            vkAllocateMemory(dev, &alloc_info, nullptr, &resbuf_mem);

            vkBindBufferMemory(dev, resbuf, resbuf_mem, 0);
        }
    }
    void BakeComputeDescriptorSet() {
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo dsl_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dsl_info.bindingCount = bindings.size();
        dsl_info.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl);

        std::vector<VkDescriptorPoolSize> pool_size(1);
        pool_size[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_size[0].descriptorCount = bindings.size();

        VkDescriptorPoolCreateInfo pool_info { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = pool_size.size();
        pool_info.pPoolSizes = pool_size.data();
        vkCreateDescriptorPool(dev, &pool_info, nullptr, &ds_pool);

        VkDescriptorSetAllocateInfo alloc_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        alloc_info.descriptorPool = ds_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &dsl;
        vkAllocateDescriptorSets(dev, &alloc_info, &ds);

        VkDescriptorBufferInfo ssbo_info {
            .buffer = dstbuf,
            .offset = 0,
            .range = SSBO_SIZE,
        };
        std::vector<VkWriteDescriptorSet> wds(1);
        wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[0].dstSet = ds;
        wds[0].dstBinding = 0;
        wds[0].dstArrayElement = 0;
        wds[0].descriptorCount = 1;
        wds[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wds[0].pBufferInfo = &ssbo_info;

        vkUpdateDescriptorSets(dev, wds.size(), wds.data(), 0, nullptr);
    }
    VkPipeline CreateComputePipeline(std::string spv) {
        VkPipeline pipeline;
        auto comp_spv = loadSPIRV(spv);
        VkShaderModule comp;
        VkShaderModuleCreateInfo shader_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shader_info.codeSize = comp_spv.size();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(comp_spv.data());
        vkCreateShaderModule(dev, &shader_info, nullptr, &comp);

        VkPipelineShaderStageCreateInfo comp_stage_info { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        comp_stage_info.flags = VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT_EXT; // force populate full subgroup size
        comp_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        comp_stage_info.module = comp;
        comp_stage_info.pName = "main";

        VkPipelineLayoutCreateInfo layout_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &dsl;
        layout_info.pushConstantRangeCount = 0;
        layout_info.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(dev, &layout_info, nullptr, &pipeline_layout);

        VkComputePipelineCreateInfo pipe_info { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        pipe_info.stage = comp_stage_info;
        pipe_info.layout = pipeline_layout;
        vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipe_info, nullptr, &pipeline);

        vkDestroyShaderModule(dev, comp, nullptr);
        return pipeline;
    }
    uint32_t findMemoryType(uint32_t memoryTypeBit, VkMemoryPropertyFlags request_prop) {
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
    void Run() {
        VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        {
            VkPipeline p = CreateComputePipeline("gl_SubgroupInvocationID.comp.spv"); pipelines.push_back(p);
            vkBeginCommandBuffer(cmdbuf, &cmdbuf_begin_info);
            vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(cmdbuf, dstbuf, 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(cmdbuf, dstbuf, resbuf, 1, &region);
            vkEndCommandBuffer(cmdbuf);

            VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmdbuf;
            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
            vkDeviceWaitIdle(dev);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, resbuf_mem, 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
            }
            vkUnmapMemory(dev, resbuf_mem);
        }
        {
            VkPipeline p = CreateComputePipeline("subgroupElect.comp.spv"); pipelines.push_back(p);
            vkBeginCommandBuffer(cmdbuf, &cmdbuf_begin_info);
            vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, p);
            vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &ds, 0, nullptr);
            // prefill magic number
            vkCmdFillBuffer(cmdbuf, dstbuf, 0, SSBO_SIZE, 66666666);
            // write to dstbuf
            vkCmdDispatch(cmdbuf, 1, 1, 1); // (1x1x1), (32x1x1)
            // copy the result back to resbuf
            VkBufferCopy region {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = SSBO_SIZE,
            };
            vkCmdCopyBuffer(cmdbuf, dstbuf, resbuf, 1, &region);
            vkEndCommandBuffer(cmdbuf);

            VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmdbuf;
            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
            vkDeviceWaitIdle(dev);
            // verify
            void *buf_ptr;
            vkMapMemory(dev, resbuf_mem, 0, SSBO_SIZE, 0, &buf_ptr);
            for (int i = 0; i < ELE_NUM; i++) {
                std::cout << *(static_cast<int *>(buf_ptr) + i) << std::endl;
            }
            vkUnmapMemory(dev, resbuf_mem);
        }
    }
private:
    VkInstance instance;
    VkPhysicalDevice pdev;
    uint32_t q_family_index;
    VkDevice dev;
    VkQueue queue;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkCommandPool cmdpool;
    VkCommandBuffer cmdbuf;
    VkDescriptorSetLayout dsl;
    VkDescriptorPool ds_pool;
    VkDescriptorSet ds;
    VkPipelineLayout pipeline_layout;
    std::vector<VkPipeline> pipelines;
    VkBuffer dstbuf, resbuf;
    VkDeviceMemory dstbuf_mem, resbuf_mem;
};

int main(int argc, char **argv) {
    SubGroup demo;
    demo.Run();
    return 0;
}
