#include <bg2e/render/vulkan/Swapchain.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/Vulkan.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

void Swapchain::init(Vulkan * vulkan, uint32_t width, uint32_t height)
{
    _vulkan = vulkan;
    create(width, height);
}

void Swapchain::resize(uint32_t width, uint32_t height)
{
    cleanup();
    create(width, height);
}

void Swapchain::cleanup()
{
    if (_vulkan)
    {
        // This images should not be cleared because they are wrappers
        // of the swapchain images and image views, that are cleared
        // later in this function
        _colorImages.clear();
        
        _depthImage->cleanup();
        delete _depthImage;
        
        vkDestroySwapchainKHR(_vulkan->device().handle(), _swapchain, nullptr);

        for (auto it = _imageViews.begin(); it != _imageViews.end(); ++it)
        {
            vkDestroyImageView(_vulkan->device().handle(), *it, nullptr);
        }
    }
}

void Swapchain::create(uint32_t width, uint32_t height)
{
    auto device = _vulkan->device();
    auto physicalDevice = _vulkan->physicalDevice();
    auto surface = _vulkan->surface();
    auto supportDetails = PhysicalDevice::SwapChainSupportDetails::get(
        physicalDevice.handle(),
        surface
    );
    
    // VK_FORMAT_B8G8R8A8_UNORM: needed to use IMGUI until we draw the UI indirectly in other image
    auto surfaceFormat = supportDetails.chooseSurfaceFormat(VK_FORMAT_B8G8R8A8_UNORM);
    auto presentMode = supportDetails.choosePresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
    auto extent = supportDetails.chooseExtent(surface);
    uint32_t imageCount = supportDetails.imageCount();
    auto graphicsFamily = device.graphicsFamily();
    auto presentFamily = device.presentFamily();
    uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };
    
    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _vulkan->surface().handle();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if (graphicsFamily != presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = supportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    VK_ASSERT(vkCreateSwapchainKHR(_vulkan->device().handle(), &createInfo, nullptr, &_swapchain));
    
    vkGetSwapchainImagesKHR(_vulkan->device().handle(), _swapchain, &imageCount, nullptr);
    _images.resize(imageCount);
    vkGetSwapchainImagesKHR(_vulkan->device().handle(), _swapchain, &imageCount, _images.data());
    _imageFormat = surfaceFormat.format;
    _extent = extent;
    
    _depthImage = Image::createAllocatedImage(
        _vulkan,
        VK_FORMAT_D32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    
    _colorImages.clear();
    _imageViews.clear();
    auto imageViewCreateInfo = Info::imageViewCreateInfo(_imageFormat, VK_NULL_HANDLE, VK_IMAGE_ASPECT_COLOR_BIT);
    for (auto i = 0; i < _images.size(); ++i)
    {
        // It's important to create the image view before wrapping the swapchain image
        VkImageView imageView;
        imageViewCreateInfo.image = _images[i];
        vkCreateImageView(device.handle(), &imageViewCreateInfo, nullptr, &imageView);
        _imageViews.push_back(imageView);
        
        _colorImages.push_back(Image::wrapSwapchainImage(this, i));
     
    }
}

}
}
}
