#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/Swapchain.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/CleanupManager.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/Surface.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>

namespace bg2e {
namespace render {

class BG2E_API Vulkan {
public:

    void init(SDL_Window * windowPtr);

    void cleanup();

    inline const void* windowPtr() const { return _windowPtr; }
    inline void* windowPtr() { return _windowPtr; }

    inline const vulkan::Instance& instance() const { return _instance; }
    inline const vulkan::PhysicalDevice& physicalDevice() const { return _physicalDevice; }
    inline const vulkan:: Surface& surface() const { return _surface; }
    inline VkDevice device() const { return _device; }
    inline vulkan::Swapchain& swapchain() { return _swapchain; }
    inline const vulkan::Swapchain& swapchain() const { return _swapchain; }
    inline vulkan::Command& command() { return _command; }
    inline const vulkan::Command& command() const { return _command; }

    inline vulkan::FrameResources& currentFrameResources() { return _frameResources[_currentFrame % vulkan::FRAME_OVERLAP]; }
    inline const vulkan::FrameResources& currentFrameResources() const { return _frameResources[_currentFrame % vulkan::FRAME_OVERLAP]; }
    inline uint32_t currentFrame() const { return _currentFrame; }
    inline void nextFrame() { ++_currentFrame; }
    void iterateFrameResources(std::function<void(vulkan::FrameResources&)> cb);

    inline vulkan::CleanupManager& cleanupManager() { return _cleanupManager; }

    inline VmaAllocator allocator() const { return _allocator; }

    inline void updateSwapchainSize() { _resizeRequested = true; }

    // This function returns true if the swapchain have been resized
    bool newFrame();

protected:
    SDL_Window* _windowPtr;

private:

    bool _debugLayers = true;

    vulkan::Instance _instance;
	vulkan::Surface _surface;
	vulkan::PhysicalDevice _physicalDevice;

    VkDevice _device = VK_NULL_HANDLE;

    vulkan::Swapchain _swapchain;
    vulkan::Command _command;

    vulkan::FrameResources _frameResources[vulkan::FRAME_OVERLAP];
    uint32_t _currentFrame = 0;

    vulkan::CleanupManager _cleanupManager;

    VmaAllocator _allocator = VK_NULL_HANDLE;

    bool _resizeRequested = false;


    void createInstance();
    void createSurface();
    void createDevicesAndQueues();
    void createMemoryAllocator();
    void createFrameResources();

    void cleanupFrameResources();
};

}
}
