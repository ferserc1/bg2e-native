#include <bg2e/render/vulkan/Device.hpp>
#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/base/Log.hpp>

#include <set>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

void Device::create(const Instance& instance, const PhysicalDevice& physicalDevice, const Surface& surface)
{
    auto indices = physicalDevice.queueFamilyIndices();
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphics.value(),
        indices.present.value()
    };
    
    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Extensions
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    
    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features = {};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    synchronization2Features.pNext = &dynamicRenderingFeatures;
    
    VkPhysicalDeviceFeatures2 features2 {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &synchronization2Features;
    
    vkGetPhysicalDeviceFeatures2(physicalDevice.handle(), &features2);
    
    if (dynamicRenderingFeatures.dynamicRendering == VK_FALSE)
    {
        throw std::runtime_error("Dynamic rendering not supported");
    }
    
    if (synchronization2Features.synchronization2 == VK_FALSE)
    {
        throw std::runtime_error("Synchronization 2 not supported");
    }
    
    features2.features.samplerAnisotropy = VK_TRUE;
    
    VkPhysicalDeviceVulkan12Features vulkan12Features {};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.pNext = &features2;
    vulkan12Features.descriptorIndexing = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    
    
    VkPhysicalDeviceVulkan11Features vulkan11Features {};
    vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan11Features.multiview = VK_TRUE;
    vulkan11Features.pNext = &vulkan12Features;
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
    createInfo.pNext = &vulkan11Features;
    
    auto deviceExtensions = PhysicalDevice::getRequiredDeviceExtensions();
    createInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    std::vector<const char*> requiredLayers;
    instance.getRequiredLayers(requiredLayers);
    if (base::Log::isDebug())
    {
        createInfo.enabledLayerCount = uint32_t(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    }
    
    VK_ASSERT(vkCreateDevice(physicalDevice.handle(), &createInfo, nullptr, &_device));
    
    _graphicsFamily = indices.graphics.value();
    _presentFamily = indices.present.value();
    vkGetDeviceQueue(_device, _graphicsFamily, 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, _presentFamily, 0, &_presentQueue);
}

void Device::cleanup()
{
    if (_device != VK_NULL_HANDLE) {
        vkDestroyDevice(_device, nullptr);
    }
}

void Device::waitIdle() const
{
    vkDeviceWaitIdle(_device);
}

}
}
}
