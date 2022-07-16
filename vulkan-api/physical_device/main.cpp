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

        uint32_t nof;
        vkEnumeratePhysicalDevices(instance, &nof, nullptr);
        std::vector<VkPhysicalDevice> pdev(nof);
        vkEnumeratePhysicalDevices(instance, &nof, pdev.data());

        for (auto it : pdev) {
            VkPhysicalDeviceProperties prop {};
            vkGetPhysicalDeviceProperties(it, &prop);
            std::cout << "vendor " << std::hex << prop.vendorID << ", name " << prop.deviceName << std::endl;

            uint32_t nof;
            vkEnumerateDeviceExtensionProperties(it, nullptr, &nof, nullptr);
            std::vector<VkExtensionProperties> extension_prop(nof);
            vkEnumerateDeviceExtensionProperties(it, nullptr, &nof, extension_prop.data());
            for (auto prop : extension_prop) {
                std::cout << "device extension: " << prop.extensionName << std::endl;
            }
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