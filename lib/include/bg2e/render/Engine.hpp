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
#include <bg2e/gpu/Instance.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/Swapchain.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/CleanupManager.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/Surface.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/Device.hpp>

#include <bg2e/gpu/vk/Instance.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace gpu::vk {
class Instance;
}

namespace render {

namespace vulkan {

class DescriptorSetAllocator;

}

class BG2E_API Engine {
public:

    void init(SDL_Window * windowPtr);

    // Offscreen application
    void init();

    void cleanup();

    inline const void* windowPtr() const { return _windowPtr; }
    inline void* windowPtr() { return _windowPtr; }

    inline gpu::Instance* instance() { return _gpuInstance.get(); }
    inline const gpu::Instance* instance() const { return _gpuInstance.get(); }
    inline const vulkan::PhysicalDevice& physicalDevice() const { return _physicalDevice; }
    const vulkan:: Surface& surface() const;
    inline const vulkan::Device& device() const { return _device; }
    vulkan::Swapchain& swapchain();
    const vulkan::Swapchain& swapchain() const;
    inline vulkan::Command& command() { return _command; }
    inline const vulkan::Command& command() const { return _command; }
    inline const vulkan::DescriptorSetAllocator& descriptorSetAllocator() const { return *_descriptorSetAllocator.get(); }
    inline vulkan::DescriptorSetAllocator& descriptorSetAllocator() { return *_descriptorSetAllocator.get(); }

    inline uint32_t numImages() const { return isOffscreen() ? 1 : _swapchain.images().size(); }
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

private:

    bool _debugLayers = true;

    std::unique_ptr<gpu::vk::Instance> _gpuInstance;

    VkInstance vkInstance() const;
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
