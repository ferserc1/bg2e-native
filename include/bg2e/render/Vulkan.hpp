#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/common.hpp>
#include <bg2e/render/Command.hpp>
#include <bg2e/render/Swapchain.hpp>
#include <bg2e/render/extensions.hpp>
#include <bg2e/render/CleanupManager.hpp>
#include <bg2e/render/FrameResources.hpp>

namespace bg2e {
namespace render {

class BG2E_API Vulkan {
public:

    void init(void * windowPtr);

    void cleanup();

    inline const void* windowPtr() const { return _windowPtr; }
    inline void* windowPtr() { return _windowPtr; }

    inline VkInstance instance() const { return _instance; }
    inline VkPhysicalDevice physicalDevice() const { return _physicalDevice; }
    inline VkDevice device() const { return _device; }
    inline VkSurfaceKHR surface() const { return _surface; }
    inline Swapchain& swapchain() { return _swapchain; }
    inline const Swapchain& swapchain() const { return _swapchain; }
    inline Command& command() { return _command; }
    inline const Command& command() const { return _command; }

    inline FrameResources& currentFrameResources() { return _frameResources[_currentFrame % FRAME_OVERLAP]; }
    inline const FrameResources& currentFrameResources() const { return _frameResources[_currentFrame % FRAME_OVERLAP]; }
    inline uint32_t currentFrame() const { return _currentFrame; }
    inline void nextFrame() { ++_currentFrame; }
    void iterateFrameResources(std::function<void(FrameResources&)> cb);

    inline CleanupManager& cleanupManager() { return _cleanupManager; }

    inline VmaAllocator allocator() const { return _allocator; }

    inline void updateSwapchainSize() { _resizeRequested = true; }

    // This function returns true if the swapchain have been resized
    bool newFrame();

protected:
    void* _windowPtr;

private:

    bool _debugLayers = true;

    vkb::Instance _vkbInstance;

    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;

    Swapchain _swapchain;
    Command _command;

    FrameResources _frameResources[FRAME_OVERLAP];
    uint32_t _currentFrame = 0;

    CleanupManager _cleanupManager;

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
