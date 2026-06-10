/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/gpu/vk/WindowSurface.hpp>
#include <bg2e/gpu/vk/Instance.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/PhysicalDevice.hpp>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <bg2e/gpu/vk/Info.hpp>
#include <bg2e/gpu/vk/extensions.hpp>
#include <bg2e/gpu/Image.hpp>

#include <SDL2/SDL_vulkan.h>

#include <algorithm>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace vk {

void WindowSurface::create(gpu::Instance* instance)
{
    auto* vkInst = dynamic_cast<vk::Instance*>(instance);
    _vkInstance = vkInst->vkInstanceHnd();
    _window = instance->window();
    SDL_Vulkan_CreateSurface(_window, _vkInstance, &_surface);

    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    _size = Size2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

void WindowSurface::cleanup()
{
    releaseRenderTarget();
    if (_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(_vkInstance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
}

uint32_t WindowSurface::width() const
{
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return static_cast<uint32_t>(w);
}

uint32_t WindowSurface::height() const
{
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return static_cast<uint32_t>(h);
}

bool WindowSurface::isValid() const
{
    return _swapchain != VK_NULL_HANDLE;
}

VkSurfaceKHR WindowSurface::handle() const
{
    return _surface;
}

SDL_Window* WindowSurface::sdlWindow() const
{
    return _window;
}

void WindowSurface::createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* physicalDevice)
{
    _device = device;
    _physicalDevice = physicalDevice;

    auto* vkDevice = dynamic_cast<vk::Device*>(device);
    auto* vkPhys = dynamic_cast<vk::PhysicalDevice*>(physicalDevice);

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhys->handle(), _surface, &caps);

    // Pick surface format
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhys->handle(), _surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhys->handle(), _surface, &formatCount, formats.data());

    VkSurfaceFormatKHR chosenFormat = formats[0];
    VkFormat preferredFormat = toVkFormat(_colorFormat);
    for (const auto& f : formats)
    {
        if (f.format == preferredFormat && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = f;
            break;
        }
    }
    _colorFormat = fromVkFormat(chosenFormat.format);

    // Present mode (FIFO always available)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Extent
    if (caps.currentExtent.width != 0xFFFFFFFF)
    {
        _size = Size2D{ caps.currentExtent.width, caps.currentExtent.height };
    }
    else
    {
        _size.width = std::clamp(_size.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        _size.height = std::clamp(_size.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    // Image count
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, caps.maxImageCount);
    }

    // Swapchain create info
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = VkExtent2D{ _size.width, _size.height };
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = _swapchain;

    VkSwapchainKHR oldSwapchain = _swapchain;
    VK_ASSERT(vkCreateSwapchainKHR(vkDevice->handle(), &createInfo, nullptr, &_swapchain));
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(vkDevice->handle(), oldSwapchain, nullptr);
    }

    // Get swapchain images
    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(vkDevice->handle(), _swapchain, &swapchainImageCount, nullptr);
    std::vector<VkImage> swapchainImages(swapchainImageCount);
    vkGetSwapchainImagesKHR(vkDevice->handle(), _swapchain, &swapchainImageCount, swapchainImages.data());

    _colorImages.clear();
    for (auto& vkImage : swapchainImages)
    {
        auto img = std::make_unique<vk::Image>();
        img->initFromSwapchainImage(vkDevice, vkImage, _colorFormat, _size);
        _colorImages.push_back(std::move(img));
    }

    // Depth image
    _depthImage.reset();
    if (_depthFormat != PixelFormat::Undefined)
    {
        _depthImage = std::make_unique<vk::Image>();
        _depthImage->buildDepthImage(vkDevice, _size, _depthFormat);
    }

    // Per-frame / per-image sync objects.
    //
    // The number of frames in flight is driven by the actual number of swapchain
    // images (it is platform/driver dependent: e.g. 3 on an Apple M-series GPU).
    // Using a fixed count smaller than the image count makes the CPU recycle
    // per-frame resources while the GPU is still using them, which the validation
    // layers report as a synchronization hazard.
    _vkDevice = vkDevice->handle();
    _framesInFlight = static_cast<uint32_t>(_colorImages.size());
    _currentFrame = 0;

    auto semInfo = Info::semaphoreCreateInfo();
    auto fenceInfo = Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    _imageAvailable.resize(_framesInFlight, VK_NULL_HANDLE);
    _inFlight.resize(_framesInFlight, VK_NULL_HANDLE);
    _frames.resize(_framesInFlight);
    for (uint32_t i = 0; i < _framesInFlight; ++i)
    {
        vkCreateSemaphore(_vkDevice, &semInfo, nullptr, &_imageAvailable[i]);
        vkCreateFence(_vkDevice, &fenceInfo, nullptr, &_inFlight[i]);
        _frames[i] = std::make_shared<vk::SurfaceFrame>();
    }

    // renderFinished is one-per-swapchain-image (waited on by present).
    _renderFinished.resize(_colorImages.size(), VK_NULL_HANDLE);
    for (auto& sem : _renderFinished)
    {
        vkCreateSemaphore(_vkDevice, &semInfo, nullptr, &sem);
    }

    // No image is in flight yet.
    _imagesInFlight.assign(_colorImages.size(), VK_NULL_HANDLE);
}

void WindowSurface::resize(const Size2D& size)
{
    setSize(size);
    releaseRenderTarget();
    createRenderTarget(_device, _physicalDevice);
}

void WindowSurface::releaseRenderTarget()
{
    if (_vkDevice != VK_NULL_HANDLE)
    {
        for (auto& fence : _inFlight)
        {
            if (fence != VK_NULL_HANDLE)
            {
                vkDestroyFence(_vkDevice, fence, nullptr);
            }
        }
        for (auto& sem : _renderFinished)
        {
            if (sem != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(_vkDevice, sem, nullptr);
            }
        }
        for (auto& sem : _imageAvailable)
        {
            if (sem != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(_vkDevice, sem, nullptr);
            }
        }
    }

    _inFlight.clear();
    _renderFinished.clear();
    _imageAvailable.clear();
    _imagesInFlight.clear();
    _frames.clear();
    _framesInFlight = 0;
    _currentFrame = 0;

    _depthImage.reset();
    _colorImages.clear();

    if (_device != nullptr)
    {
        auto* vkDevice = dynamic_cast<vk::Device*>(_device);
        if (_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(vkDevice->handle(), _swapchain, nullptr);
            _swapchain = VK_NULL_HANDLE;
        }
    }
}

uint32_t WindowSurface::imageCount() const
{
    return static_cast<uint32_t>(_colorImages.size());
}

gpu::Image* WindowSurface::colorImage(uint32_t index) const
{
    if (index < _colorImages.size())
    {
        return _colorImages[index].get();
    }
    return nullptr;
}

gpu::Image* WindowSurface::depthImage() const
{
    return _depthImage.get();
}

std::shared_ptr<gpu::SurfaceFrame> WindowSurface::beginFrame()
{
    vkWaitForFences(_vkDevice, 1, &_inFlight[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult r = acquireNextImage(_vkDevice, _swapchain, UINT64_MAX,
        _imageAvailable[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (r == VK_ERROR_OUT_OF_DATE_KHR)
    {
        resize(_size);
        return beginFrame();
    }

    // If a previous frame is still rendering into this image, wait for it before
    // reusing the image.
    if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(_vkDevice, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    _imagesInFlight[imageIndex] = _inFlight[_currentFrame];

    auto frame = _frames[_currentFrame];
    frame->setColorImage(_colorImages[imageIndex].get());
    frame->setDepthImage(_depthImage.get());
    frame->setImageIndex(imageIndex);
    frame->setSwapchain(_swapchain);
    frame->setImageAvailable(_imageAvailable[_currentFrame]);
    frame->setRenderFinished(_renderFinished[imageIndex]);
    frame->setInFlightFence(_inFlight[_currentFrame]);
    return frame;
}

void WindowSurface::present(gpu::CommandBuffer* cmd)
{
    auto* vkCmd = dynamic_cast<vk::CommandBuffer*>(cmd);
    vkCmd->setPresentFrame(_frames[_currentFrame].get());
}

void WindowSurface::endFrame(gpu::SurfaceFrame*)
{
    _currentFrame = (_currentFrame + 1) % _framesInFlight;
}

VkSwapchainKHR WindowSurface::swapchain() const
{
    return _swapchain;
}

}
}
}
