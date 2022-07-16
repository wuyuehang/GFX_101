#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

class SkeletonVulkan {
public:
    SkeletonVulkan() {
        VkApplicationInfo appInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "SkeletonVulkan",
            .applicationVersion = 0,
            .pEngineName = "SkeletonVulkan",
            .engineVersion = 0,
            .apiVersion = VK_API_VERSION_1_0,
        };
        VkInstanceCreateInfo instInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = nullptr,
        };
        vkCreateInstance(&instInfo, nullptr, &instance);

        uint32_t nof_instance_layer;
        vkEnumerateInstanceLayerProperties(&nof_instance_layer, nullptr);
        std::vector<VkLayerProperties> instance_layer_prop(nof_instance_layer);
        vkEnumerateInstanceLayerProperties(&nof_instance_layer, instance_layer_prop.data());
        for (auto it : instance_layer_prop) {
            std::cout << "layer name: " << it.layerName << std::endl;
            //std::cout << "description: " << it.description << std::endl;
        }

        uint32_t nof_instance_extension;
        vkEnumerateInstanceExtensionProperties(nullptr, &nof_instance_extension, nullptr);
        std::vector<VkExtensionProperties> instance_extension_prop(nof_instance_extension);
        vkEnumerateInstanceExtensionProperties(nullptr, &nof_instance_extension, instance_extension_prop.data());
        for (auto it : instance_extension_prop) {
            std::cout << "extension name: " << it.extensionName << std::endl;
        }
    }
    ~SkeletonVulkan() {
        vkDestroyInstance(instance, nullptr);
    }
private:
    VkInstance instance;
};

int main() {
    SkeletonVulkan T;
    return 0;
}