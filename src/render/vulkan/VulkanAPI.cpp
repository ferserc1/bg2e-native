
#include <bg2e/render/vulkan/VulkanAPI.hpp>

#include <bg2e/render/vulkan/tools.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <tuple>

namespace bg2e {
namespace render {
namespace vulkan {

void VulkanAPI::init(bool validationLayers, const std::string& appName, app::Window& window, uint32_t simultaneousFrames)
{
    _enableValidationLayers = validationLayers;
    _simultaneousFrames = simultaneousFrames;
    _currentFrame = 0;
    
    if (_enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layer requested, but not available");
    }
    
    vk::ApplicationInfo appInfo(
        appName.c_str(), 1,
        "business-grade graphic engine", 1,
        VK_API_VERSION_1_3);
    
    _instance = createInstance(appInfo, _enableValidationLayers);
    destroyManager.push_function([=]() {
        _instance.destroy();
    });
    
    if (_enableValidationLayers)
    {
        _debugMessenger = setupDebugMessenger(_instance);
        destroyManager.push_function([=]() {
            destroyDebugMessenger(_instance, _debugMessenger);
        });
    }
    
    auto win = reinterpret_cast<GLFWwindow*>(window.impl_ptr());
    
    int width;
    int height;
    glfwGetWindowSize(win, &width, &height);
    
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance(), win, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create Vulkan surface.");
    }
    _surface = surface;
    destroyManager.push_function([&]() {
        instance().destroySurfaceKHR(_surface);
    });
    
    _physicalDevice = pickPhysicalDevice(_instance, _surface);

    QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);
    _device = createDevice(_instance, _physicalDevice, _surface, _enableValidationLayers);
    _presentQueue = _device.getQueue(indices.presentFamily.value(), 0);
    _graphicsQueue = _device.getQueue(indices.graphicsFamily.value(), 0);
    destroyManager.push_function([&]() {
        _device.destroy();
    });


    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = _physicalDevice;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
    destroyManager.push_function([=]() {
        vmaDestroyAllocator(_allocator);
    });
    
    createSwapChain(_instance, _physicalDevice, _device, _surface, window, _swapChain);
    destroyManager.push_function([&]() {
        destroySwapChain(_device, _swapChain);
    });

    _mainRenderPass = createBasicDepthRenderPass(_physicalDevice, _device, _swapChain.imageFormat);
    destroyManager.push_function([&]() {
        _device.destroyRenderPass(_mainRenderPass);
    });
    
    vk::CommandPool immediateCmdPool = createCommandPool(_device, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, indices.graphicsFamily.value());
    vk::Queue immediateQueue = _device.getQueue(indices.graphicsFamily.value(), 0);
    vk::CommandBuffer immediateCmdBuffer = allocateCommandBuffer(_device, immediateCmdPool, vk::CommandBufferLevel::ePrimary);
    _cmdExec = std::make_unique<ImmediateCommandBuffer>(_device, immediateCmdPool, immediateQueue, immediateCmdBuffer);
    destroyManager.push_function([=]() {
        _cmdExec = nullptr;
        _device.destroyCommandPool(immediateCmdPool);
    });

    _depthResources = createDepthResources(_allocator, _physicalDevice, _device, _swapChain, *(_cmdExec.get()));
    destroyManager.push_function([&]() {
        destroyDepthResources(_allocator, _device, _depthResources);
    });

    createFramebuffers(_device, _mainRenderPass, _swapChain, _depthResources, _framebuffers);
    destroyManager.push_function([&]() {
        destroyFramebuffers(_device, _framebuffers);
    });
    
    _commandPool = createCommandPool(_device, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, indices.graphicsFamily.value());
    
    allocateCommandBuffers(_device, _commandPool, vk::CommandBufferLevel::ePrimary, _simultaneousFrames, _commandBuffers);
    destroyManager.push_function([&]() {
        _device.destroyCommandPool(_commandPool);
    });
    
    createFrameSyncResources(_device, _simultaneousFrames, _frameSyncResources);
    destroyManager.push_function([&]() {
        destroyFrameSyncResources(_device, _frameSyncResources);
    });
}

int32_t VulkanAPI::beginFrame()
{
    if (_device.waitForFences(1, &_frameSyncResources[_currentFrame].renderFence, true, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for fences");
    }
    
    uint32_t imageIndex;
    vk::Result result;
    try {
        std::tie(result, imageIndex) = _device.acquireNextImageKHR(_swapChain.swapchain, UINT64_MAX, _frameSyncResources[_currentFrame].presentSemaphore, nullptr);
    }
    catch (vk::OutOfDateKHRError&)
    {
        result = vk::Result::eErrorOutOfDateKHR;
    }
    
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        // TODO: Recreate swap chain
        return 0;
    }
    
    // TODO: Update uniform buffers and other structures
    
    if (_device.resetFences(1, &_frameSyncResources[_currentFrame].renderFence) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to reset fence");
    }
    
    _commandBuffers[_currentFrame].reset({});
    return imageIndex;
}

void VulkanAPI::endFrame(int32_t swapChainImageIndex)
{
    // Submit buffers
    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_frameSyncResources[_currentFrame].presentSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_frameSyncResources[_currentFrame].renderSemaphore;
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];
    
    if (_graphicsQueue.submit(1, &submitInfo, _frameSyncResources[_currentFrame].renderFence) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit command buffer");
    }
    
    // Present frame
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_frameSyncResources[_currentFrame].renderSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapChain.swapchain;
    uint32_t indices[] = { static_cast<uint32_t>(swapChainImageIndex) };
    presentInfo.pImageIndices = indices;
    presentInfo.pResults = nullptr;
    
    vk::Result result;
    try {
        result = _presentQueue.presentKHR(presentInfo);
    }
    catch (vk::OutOfDateKHRError&)
    {
        result = vk::Result::eErrorOutOfDateKHR;
    }
    
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        // _framebufferResized = false;
        // recreateSwapChain();
    }
    
    _currentFrame = (_currentFrame + 1) % _simultaneousFrames;
}

void VulkanAPI::destroy()
{
    for (int i = 0; i < _simultaneousFrames; ++i)
    {
        _device.waitForFences(1, &_frameSyncResources[i].renderFence, true, UINT64_MAX);
    }
    
    _device.waitIdle();
    destroyManager.flush();
}




}
}
}
