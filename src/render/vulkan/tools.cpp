
#include <bg2e/render/vulkan/tools.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

namespace bg2e {
namespace render {
namespace vulkan {

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }
    
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

    createInfo.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
}


const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> g_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
#ifdef __APPLE__
    ,
    "VK_KHR_portability_subset"
#endif
};

bool checkValidationLayerSupport()
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    for (const char* layerName : g_validationLayers)
    {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound)
        {
            return false;
        }
    }
    
    return true;
}

std::vector<const char*> getRequiredExtensions(bool validationLayersSupport)
{
    
    
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    
    std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if (validationLayersSupport)
    {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    
    return requiredExtensions;
}

vk::Instance createInstance(vk::ApplicationInfo& appInfo, bool validationLayersSupport)
{
    vk::InstanceCreateInfo createInfo({}, &appInfo);
    
    auto requiredExtensions = getRequiredExtensions(validationLayersSupport);
    
#ifdef __APPLE__
    createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
 
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (validationLayersSupport)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    
    return vk::createInstance(createInfo, nullptr);
}

VkDebugUtilsMessengerEXT setupDebugMessenger(vk::Instance instance)
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    VkDebugUtilsMessengerEXT debugMessenger;
    
    if (CreateDebugUtilsMessengerEXT(
         instance,
         reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
         nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to setup debug messenger");
    }
    
    return debugMessenger;
}

void destroyDebugMessenger(vk::Instance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}


}
}
}
