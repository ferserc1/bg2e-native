#include <bg2e/render/vulkan/Swapchain.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

void Swapchain::init(Engine * engine, uint32_t width, uint32_t height)
{
    _engine = engine;
    create(width, height);
}

void Swapchain::resize(uint32_t width, uint32_t height)
{
    cleanup();
    create(width, height);
}

void Swapchain::cleanup()
{
    if (_engine)
    {
        // This images should not be cleared because they are wrappers
        // of the swapchain images and image views, that are cleared
        // later in this function
        _colorImages.clear();
        
        _depthImage->cleanup();
        delete _depthImage;
        
        vkDestroySwapchainKHR(_engine->device().handle(), _swapchain, nullptr);

        for (auto it = _imageViews.begin(); it != _imageViews.end(); ++it)
        {
            vkDestroyImageView(_engine->device().handle(), *it, nullptr);
        }
    }
    _swapchain = VK_NULL_HANDLE;
}

void Swapchain::create(uint32_t width, uint32_t height)
{
    auto device = _engine->device();
    auto physicalDevice = _engine->physicalDevice();
    auto surface = _engine->surface();
    auto supportDetails = PhysicalDevice::SwapChainSupportDetails::get(
        physicalDevice.handle(),
        surface
    );
    
    auto surfaceFormat = supportDetails.chooseSurfaceFormat(VK_FORMAT_R8G8B8A8_UNORM);
    auto presentMode = supportDetails.choosePresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
    auto extent = supportDetails.chooseExtent(surface);
    uint32_t imageCount = supportDetails.imageCount();
    auto graphicsFamily = device.graphicsFamily();
    auto presentFamily = device.presentFamily();
    uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };
    
    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _engine->surface().handle();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
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
    
    VK_ASSERT(vkCreateSwapchainKHR(_engine->device().handle(), &createInfo, nullptr, &_swapchain));
    
    vkGetSwapchainImagesKHR(_engine->device().handle(), _swapchain, &imageCount, nullptr);
    _images.resize(imageCount);
    vkGetSwapchainImagesKHR(_engine->device().handle(), _swapchain, &imageCount, _images.data());
    _imageFormat = surfaceFormat.format;
    _extent = extent;
    
    _depthImage = Image::createAllocatedImage(
        _engine,
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
