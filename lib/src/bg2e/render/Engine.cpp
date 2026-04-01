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

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/extensions.hpp>

#ifdef BG2E_LINUX

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#else

#include <SDL2/SDL.h>
#include <SDL_vulkan.h>

#endif

namespace bg2e {
namespace render {

void Engine::init(SDL_Window* windowPtr)
{
    _windowPtr = windowPtr;

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(_windowPtr, &width, &height);

    createInstance();
    vulkan::loadExtensions(_instance.handle());
    createSurface();
    createDevicesAndQueues();
    createMemoryAllocator();
    
    _swapchain.init(this, uint32_t(width), uint32_t(height));
    
    createFrameResources();
    
    // Create main descriptor set allocator
    _descriptorSetAllocator = std::unique_ptr<vulkan::DescriptorSetAllocator>(
        new vulkan::DescriptorSetAllocator()
    );
    _descriptorSetAllocator->init(this);
    _cleanupManager.push([&](VkDevice) {
        _descriptorSetAllocator.reset();
    });
}

void Engine::cleanup()
{
    _device.waitIdle();

    _cleanupManager.flush(_device);

    cleanupFrameResources();

    _swapchain.cleanup();

    vmaDestroyAllocator(_allocator);

    _device.cleanup();
    _surface.cleanup();
    _instance.cleanup();
}

bool Engine::newFrame()
{
    if (_resizeRequested)
    {
        _device.waitIdle();

        int w, h;
        SDL_GetWindowSize(_windowPtr, &w, &h);
        _swapchain.resize(uint32_t(w), uint32_t(h));

        _resizeRequested = false;
        return true;
    }

    return false;
}

void Engine::createInstance()
{
    _instance.create(_windowPtr);
}

void Engine::createSurface()
{
    _surface.create(_instance, _windowPtr);
}

void Engine::createDevicesAndQueues()
{
	_physicalDevice.choose(_instance, _surface);
    _device.create(_instance, _physicalDevice, _surface);
    _command.init(this);
}

void Engine::createMemoryAllocator()
{
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physicalDevice.handle();
    allocInfo.device = _device.handle();
    allocInfo.instance = _instance.handle();
    allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocInfo, &_allocator);
}

void Engine::createFrameResources()
{
    auto numImages = swapchain().images().size();
    _frameResources.resize(numImages);
    for (uint32_t i = 0; i < numImages; ++i)
    {
        _frameResources[i].init(_device, &_command);
    }
}

void Engine::cleanupFrameResources()
{
    auto numImages = swapchain().images().size();
    for (uint32_t i = 0; i < numImages; ++i)
    {
        _frameResources[i].cleanup();
    }
}

void Engine::iterateFrameResources(std::function<void(vulkan::FrameResources&)> cb)
{
    size_t numImages = swapchain().images().size();
    for (size_t i = 0; i < numImages; ++i)
    {
        cb(_frameResources[i]);
    }
}

void Engine::destroyBuffer(VkBuffer buffer, VmaAllocation allocation)
{
    vmaDestroyBuffer(_allocator, buffer, allocation);
}

}
}
