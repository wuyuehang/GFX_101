#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define BUFFER_COPY_SIZE 64

class HelloVulkan {
public:
    HelloVulkan() {
        CreateInstance();
        CreateDevice();
        CreateCommandBuffer();
        CreateShaderStorageBuffer();
        BakeComputeDescriptorSet();
        CreateComputePipeline();
        BakeComputeCommand();
    }
    ~HelloVulkan() {
        vkDeviceWaitIdle(dev);
        vkDestroyPipeline(dev, comp_pipeline, nullptr);
        vkDestroyPipelineLayout(dev, comp_layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, comp_dsl, nullptr);
        vkFreeDescriptorSets(dev, comp_ds_pool, 1, &comp_ds);
        vkDestroyDescriptorPool(dev, comp_ds_pool, nullptr);
        vkFreeMemory(dev, dstbuf_mem, nullptr);
        vkFreeMemory(dev, resbuf_mem, nullptr);
        vkDestroyBuffer(dev, dstbuf, nullptr);
        vkDestroyBuffer(dev, resbuf, nullptr);
        vkFreeCommandBuffers(dev, cmdpool, 1, &comp_cmdbuf);
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
        VkApplicationInfo app_info {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo instance_info {};
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &app_info;
        vkCreateInstance(&instance_info, nullptr, &instance);
    }
    void CreateDevice() {
        uint32_t pdev_count;
        vkEnumeratePhysicalDevices(instance, &pdev_count, nullptr);
        std::vector<VkPhysicalDevice> physical_dev(pdev_count);
        vkEnumeratePhysicalDevices(instance, &pdev_count, physical_dev.data());
        pdev = physical_dev[0];

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

        VkDeviceQueueCreateInfo queue_info {};
        float q_prio = 1.0;
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = q_family_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &q_prio;

        VkDeviceCreateInfo dev_info {};
        dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dev_info.queueCreateInfoCount = 1;
        dev_info.pQueueCreateInfos = &queue_info;

        vkCreateDevice(pdev, &dev_info, nullptr, &dev);
        //VkBool32 present_supported = VK_FALSE;
        //vkGetPhysicalDeviceSurfaceSupportKHR(pdev, q_family_index, surface, &present_supported);
        //assert(VK_TRUE == present_supported); // assume the render queue is the same as present queue
        vkGetDeviceQueue(dev, q_family_index, 0, &queue); // we only create 1 queue from the family queues, its index is 0

        vkGetPhysicalDeviceMemoryProperties(pdev, &mem_properties);
    }
    void CreateCommandBuffer() {
        VkCommandPoolCreateInfo commandpool_info {};
        commandpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow individual commandbuffer reset
        commandpool_info.queueFamilyIndex = q_family_index;
        vkCreateCommandPool(dev, &commandpool_info, nullptr, &cmdpool);

        {
            VkCommandBufferAllocateInfo cmdbuf_info {};
            cmdbuf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdbuf_info.commandPool = cmdpool;
            cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdbuf_info.commandBufferCount = 1;
            vkAllocateCommandBuffers(dev, &cmdbuf_info, &comp_cmdbuf);
        }
    }
    void CreateShaderStorageBuffer() {
        // initialize dst storage buffer
        VkBufferCreateInfo buf_info {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.size = BUFFER_COPY_SIZE * sizeof(int);
        buf_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(dev, &buf_info, nullptr, &dstbuf);

        VkMemoryRequirements req {};
        vkGetBufferMemoryRequirements(dev, dstbuf, &req);

        VkMemoryAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(dev, &alloc_info, nullptr, &dstbuf_mem);

        vkBindBufferMemory(dev, dstbuf, dstbuf_mem, 0);

        {
            // alloc a linear buffer for verifcation
            VkBufferCreateInfo buf_info {};
            buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buf_info.size = BUFFER_COPY_SIZE * sizeof(int);
            buf_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(dev, &buf_info, nullptr, &resbuf);

            VkMemoryRequirements req {};
            vkGetBufferMemoryRequirements(dev, resbuf, &req);

            VkMemoryAllocateInfo alloc_info {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
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

        VkDescriptorSetLayoutCreateInfo dsl_info {};
        dsl_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dsl_info.bindingCount = bindings.size();
        dsl_info.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &comp_dsl);

        std::vector<VkDescriptorPoolSize> pool_size(1);
        pool_size[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_size[0].descriptorCount = bindings.size();

        VkDescriptorPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = pool_size.size();
        pool_info.pPoolSizes = pool_size.data();
        vkCreateDescriptorPool(dev, &pool_info, nullptr, &comp_ds_pool);

        VkDescriptorSetAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = comp_ds_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &comp_dsl;
        vkAllocateDescriptorSets(dev, &alloc_info, &comp_ds);

        VkDescriptorBufferInfo ssbo_info {
            .buffer = dstbuf,
            .offset = 0,
            .range = BUFFER_COPY_SIZE * sizeof(int),
        };
        std::vector<VkWriteDescriptorSet> wds(1);
        wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[0].dstSet = comp_ds;
        wds[0].dstBinding = 0;
        wds[0].dstArrayElement = 0;
        wds[0].descriptorCount = 1;
        wds[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wds[0].pBufferInfo = &ssbo_info;

        vkUpdateDescriptorSets(dev, wds.size(), wds.data(), 0, nullptr);
    }
    void CreateComputePipeline() {
        auto comp_spv = loadSPIRV("ssbo.comp.spv");
        VkShaderModule comp;
        VkShaderModuleCreateInfo shader_info {};
        shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_info.codeSize = comp_spv.size();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(comp_spv.data());
        vkCreateShaderModule(dev, &shader_info, nullptr, &comp);

        VkPipelineShaderStageCreateInfo comp_stage_info {};
        comp_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        comp_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        comp_stage_info.module = comp;
        comp_stage_info.pName = "main";

        VkPipelineLayoutCreateInfo layout_info {};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &comp_dsl;
        layout_info.pushConstantRangeCount = 0;
        layout_info.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(dev, &layout_info, nullptr, &comp_layout);

        VkComputePipelineCreateInfo pipe_info {};
        pipe_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipe_info.stage = comp_stage_info;
        pipe_info.layout = comp_layout;
        vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipe_info, nullptr, &comp_pipeline);

        vkDestroyShaderModule(dev, comp, nullptr);
    }
    void BakeComputeCommand() {
        VkCommandBufferBeginInfo cmdbuf_begin_info {};
        cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        vkBeginCommandBuffer(comp_cmdbuf, &cmdbuf_begin_info);
        vkCmdBindPipeline(comp_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, comp_pipeline);
        vkCmdBindDescriptorSets(comp_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, comp_layout, 0, 1, &comp_ds, 0, nullptr);
        // write to dstbuf
        vkCmdDispatch(comp_cmdbuf, 1, 1, 1); // (1x1x1), (64x1x1)
        // copy the result back to resbuf
        VkBufferCopy region {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = BUFFER_COPY_SIZE * sizeof(int),
        };
        vkCmdCopyBuffer(comp_cmdbuf, dstbuf, resbuf, 1, &region);
        vkEndCommandBuffer(comp_cmdbuf);
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
        VkFence fence;
        VkFenceCreateInfo fence_info {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(dev, &fence_info, nullptr, &fence);

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &comp_cmdbuf;
        vkQueueSubmit(queue, 1, &submit_info, fence);

        vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);

        {
            void *buf_ptr;
            vkMapMemory(dev, resbuf_mem, 0, BUFFER_COPY_SIZE, 0, &buf_ptr);
            for (int i = 0; i < BUFFER_COPY_SIZE; i++) {
                assert(*(static_cast<int *>(buf_ptr) + i) == 0x01020304);
            }
            vkUnmapMemory(dev, resbuf_mem);
        }

        vkDestroyFence(dev, fence, nullptr);
    }
private:
    VkInstance instance;
    VkPhysicalDevice pdev;
    uint32_t q_family_index;
    VkDevice dev;
    VkQueue queue;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkCommandPool cmdpool;
    VkCommandBuffer comp_cmdbuf;
    VkDescriptorSetLayout comp_dsl;
    VkDescriptorPool comp_ds_pool;
    VkDescriptorSet comp_ds;
    VkPipelineLayout comp_layout;
    VkPipeline comp_pipeline;
    VkBuffer dstbuf, resbuf;
    VkDeviceMemory dstbuf_mem, resbuf_mem;
};

int main(int argc, char **argv) {
    HelloVulkan demo;
    demo.Run();
    return 0;
}
