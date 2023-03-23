
#include <bg2e/render/vulkan/tools.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>

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

bool checkDeviceExtensions(vk::PhysicalDevice device)
{
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties(nullptr);
    std::set<std::string> requiredExtensions(g_deviceExtensions.begin(), g_deviceExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
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

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices;
    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        
        if (device.getSurfaceSupportKHR(i, surface))
        {
            indices.presentFamily = i;
        }
        
        if (indices.isComplete())
        {
            break;
        }
        
        ++i;
    }
    return indices;
}

bool isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    bool extensionsSupported = checkDeviceExtensions(device);
    
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }
    
    return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentFormat(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, app::Window& window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        auto win = reinterpret_cast<GLFWwindow*>(window.impl_ptr());
        int width, height;
        glfwGetFramebufferSize(win, &width, &height);
        
        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        
        return actualExtent;
    }
}

vk::PhysicalDevice pickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
{
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    
    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device, surface))
        {
            return device;
        }
    }
    
    throw std::runtime_error("Failed to pick a suitable GPU");
}

vk::Device createDevice(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, bool enableValidationLayers)
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies{ indices.graphicsFamily.value(), indices.presentFamily.value() };
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    vk::PhysicalDeviceFeatures deviceFeatures;
    
    vk::DeviceCreateInfo createInfo;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();
    
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
    }
    
    return physicalDevice.createDevice(createInfo);
}

void createSwapChain(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface, app::Window& window, SwapChainResources& result)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
    result.surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    result.presentMode = chooseSwapPresentFormat(swapChainSupport.presentModes);
    result.extent = chooseSwapExtent(swapChainSupport.capabilities, window);
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = result.surfaceFormat.format;
    createInfo.imageColorSpace = result.surfaceFormat.colorSpace;
    createInfo.imageExtent = result.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };
    
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = result.presentMode;
    createInfo.clipped = true;
    createInfo.oldSwapchain = nullptr;
    
    result.swapchain = device.createSwapchainKHR(createInfo);
    result.images = device.getSwapchainImagesKHR(result.swapchain);
    result.imageFormat = result.surfaceFormat.format;
    
    // Create image views
    for (auto image : result.images)
    {
        vk::ImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.image = image;
        viewCreateInfo.viewType = vk::ImageViewType::e2D;
        viewCreateInfo.format = result.imageFormat;
        
        viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        
        viewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;
        
        result.imageViews.push_back(device.createImageView(viewCreateInfo));
    }
}

void destroySwapChain(vk::Device device, SwapChainResources& swapchainData)
{
    for (auto imageView : swapchainData.imageViews)
    {
        device.destroyImageView(imageView);
    }
    
    device.destroySwapchainKHR(swapchainData.swapchain);
    
    swapchainData.swapchain = nullptr;
    swapchainData.images.clear();
    swapchainData.imageViews.clear();
    swapchainData.extent.width = 0;
    swapchainData.extent.height = 0;
}

}
}
}
