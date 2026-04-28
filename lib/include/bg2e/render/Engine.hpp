/*
 *    business grade graphic engine (bg2 engine)
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

#pragma once

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
#include <bg2e/render/vulkan/Device.hpp>

namespace bg2e {
namespace render {

namespace vulkan {

class DescriptorSetAllocator;

}

class BG2E_API Engine {
public:

    void init(SDL_Window * windowPtr);

    // Offscreen application
    void init(uint32_t width, uint32_t height);

    void cleanup();

    inline const void* windowPtr() const { return _windowPtr; }
    inline void* windowPtr() { return _windowPtr; }

    inline const vulkan::Instance& instance() const { return _instance; }
    inline const vulkan::PhysicalDevice& physicalDevice() const { return _physicalDevice; }
    const vulkan:: Surface& surface() const;
    inline const vulkan::Device& device() const { return _device; }
    vulkan::Swapchain& swapchain();
    const vulkan::Swapchain& swapchain() const;
    inline vulkan::Command& command() { return _command; }
    inline const vulkan::Command& command() const { return _command; }
    inline const vulkan::DescriptorSetAllocator& descriptorSetAllocator() const { return *_descriptorSetAllocator.get(); }
    inline vulkan::DescriptorSetAllocator& descriptorSetAllocator() { return *_descriptorSetAllocator.get(); }

    uint32_t currentFrameResourcesIndex() const;
    uint32_t prevFrameResourcesIndex() const;
    vulkan::FrameResources& currentFrameResources();
    const vulkan::FrameResources& currentFrameResources() const;

    inline uint32_t currentFrame() const { return _currentFrame; }
    inline void nextFrame() { ++_currentFrame; }
    void iterateFrameResources(std::function<void(vulkan::FrameResources&)> cb);

    inline vulkan::CleanupManager& cleanupManager() { return _cleanupManager; }

    inline VmaAllocator allocator() const { return _allocator; }

    inline bool rayTracingSupported() const { return _physicalDevice.properties()->rayTracingSupported(); }

    inline void updateSwapchainSize() { _resizeRequested = true; }

    // This function returns true if the swapchain have been resized
    bool newFrame();

	// Destroy a buffer allocated with the Vulkan Memory Allocator
	void destroyBuffer(VkBuffer buffer, VmaAllocation allocation);

    bool isOffscreen() const { return _windowPtr == nullptr; }

protected:
    SDL_Window* _windowPtr = nullptr;
    uint32_t _offscreenWidth = 0;
    uint32_t _offscreenHeight = 0;

private:

    bool _debugLayers = true;

    vulkan::Instance _instance;
	vulkan::Surface _surface;
	vulkan::PhysicalDevice _physicalDevice;
    vulkan::Device _device;
    vulkan::Swapchain _swapchain;
    vulkan::Command _command;
    
    std::unique_ptr<vulkan::DescriptorSetAllocator> _descriptorSetAllocator;

    std::vector<vulkan::FrameResources> _frameResources;
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

// Resolve the forward declaration of DescriptorSetAllocator
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
