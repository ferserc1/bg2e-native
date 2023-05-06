
#include <bg2e/render/vulkan/tools.hpp>

#include <bg2e/render/vulkan/ImmediateCommandBuffer.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>

// Implementation of Vulkan Memory Allocator
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

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
    
#ifdef __APPLE__
    requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

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
        result.imageViews.push_back(createImageView(device, image, result.imageFormat, vk::ImageAspectFlagBits::eColor));
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

vk::Format findSupportedFormat(vk::PhysicalDevice device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (auto format : candidates)
    {
        vk::FormatProperties props = device.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear &&
            (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format");
}

vk::Format findDepthFormat(vk::PhysicalDevice device)
{
    return findSupportedFormat(
        device,
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::RenderPass createBasicDepthRenderPass(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Format colorFormat)
{
    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlagBits::eNone;
    dependency.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentRead;

    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = colorFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = findDepthFormat(physicalDevice);
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    std::array<vk::AttachmentDescription, 2> attachments = {
        colorAttachment,
        depthAttachment
    };

    vk::RenderPassCreateInfo createInfo;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    return device.createRenderPass(createInfo);
}

AllocatedImage createImage(VmaAllocator allocator, vk::Device device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryFlags)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = static_cast<VkFormat>(format);
    imageInfo.tiling = static_cast<VkImageTiling>(tiling);
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo img_allocInfo{};
    img_allocInfo.usage = memoryUsage;
    img_allocInfo.flags = memoryFlags;
    AllocatedImage newImage;
    VkImage image;
    vmaCreateImage(allocator, &imageInfo, &img_allocInfo, &image, &newImage.allocation, nullptr);
    newImage.image = image;
    return newImage;
}

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return device.createImageView(viewInfo);
}

void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, ImmediateCommandBuffer& cmdExec)
{
    cmdExec.execute([&](vk::CommandBuffer cmd) {
        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;
        
        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            
            // If has stencil component
            if (format == vk::Format::eD32SfloatS8Uint ||
                format == vk::Format::eD24UnormS8Uint)
            {
                barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        }
        
        if (oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
                 newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else if (oldLayout == vk::ImageLayout::eUndefined &&
                 newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask =
                vk::AccessFlagBits::eDepthStencilAttachmentRead |
                vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        else
        {
            throw std::invalid_argument("Unsupported layout transition");
        }
        
        cmd.pipelineBarrier(
            sourceStage,
            destinationStage,
            {},
            0, nullptr,
            0, nullptr,
            1, &barrier);
    });
}

DepthResources createDepthResources(VmaAllocator allocator, vk::PhysicalDevice physicalDevice, vk::Device device, const SwapChainResources& swapChainData, ImmediateCommandBuffer& cmdExec)
{
    DepthResources result;
    vk::Format depthFormat = findDepthFormat(physicalDevice);
    result.image = createImage(allocator, device, swapChainData.extent.width, swapChainData.extent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    result.view = createImageView(device, result.image.image, depthFormat, vk::ImageAspectFlagBits::eDepth);

    transitionImageLayout(
          result.image.image,
          depthFormat,
          vk::ImageLayout::eUndefined,
          vk::ImageLayout::eDepthStencilAttachmentOptimal,
          cmdExec);

    return result;
}

void destroyDepthResources(VmaAllocator allocator, vk::Device device, DepthResources& res)
{
    vmaDestroyImage(allocator, res.image.image, res.image.allocation);
    device.destroyImageView(res.view);
}

void createFramebuffers(vk::Device device, vk::RenderPass renderPass, const SwapChainResources& swapchainData, const DepthResources& depthData, std::vector<vk::Framebuffer>& result)
{
    for (auto imageView : swapchainData.imageViews)
    {
        std::array<vk::ImageView, 2> attachments = {
            imageView,
            depthData.view
        };
        
        vk::FramebufferCreateInfo createInfo;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.width = swapchainData.extent.width;
        createInfo.height = swapchainData.extent.height;
        createInfo.layers = 1;
        
        result.push_back(device.createFramebuffer(createInfo));
    }
}

void destroyFramebuffers(vk::Device device, std::vector<vk::Framebuffer>& framebuffers)
{
    for (auto fb : framebuffers)
    {
        device.destroyFramebuffer(fb);
    }
    framebuffers.clear();
}

vk::CommandPool createCommandPool(vk::Device device, vk::CommandPoolCreateFlags flags, uint32_t queueFamily)
{
    vk::CommandPoolCreateInfo createInfo;
    createInfo.flags = flags;
    createInfo.queueFamilyIndex = queueFamily;
    return device.createCommandPool(createInfo);
}

void allocateCommandBuffers(vk::Device device, vk::CommandPool pool, vk::CommandBufferLevel level, uint32_t bufferCount, std::vector<vk::CommandBuffer>& result)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = bufferCount;
    result = device.allocateCommandBuffers(allocInfo);
}

vk::CommandBuffer allocateCommandBuffer(vk::Device device, vk::CommandPool pool, vk::CommandBufferLevel level)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;
    return device.allocateCommandBuffers(allocInfo)[0];
}

void createFrameSyncResources(vk::Device device, uint32_t frameCount, std::vector<FrameSync>& result, vk::FenceCreateFlags fenceFlags)
{
    for (uint32_t i = 0; i < frameCount; ++i)
    {
        vk::FenceCreateInfo fenceInfo;
        fenceInfo.flags = fenceFlags;
        vk::Fence fence = device.createFence(fenceInfo);
        
        vk::SemaphoreCreateInfo semaphoreInfo;
        auto presentSemaphore = device.createSemaphore(semaphoreInfo);
        auto renderSemaphore = device.createSemaphore(semaphoreInfo);
        result.push_back({
            fence,
            presentSemaphore,
            renderSemaphore
        });
    }
}

void destroyFrameSyncResources(vk::Device device, std::vector<FrameSync>& syncResources)
{
    for (auto sync : syncResources)
    {
        device.destroySemaphore(sync.renderSemaphore);
        device.destroySemaphore(sync.presentSemaphore);
        device.destroyFence(sync.renderFence);
    }
    syncResources.clear();
}

}
}
}
