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
#include <stdexcept>

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
    vulkan::loadInstanceExtensions(_instance.handle(), false);
    createSurface();
    createDevicesAndQueues();
    createMemoryAllocator();

    vulkan::loadDeviceExtensions(_physicalDevice, _device.handle(), false);
    
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

void Engine::init()
{

    createInstance();
    vulkan::loadInstanceExtensions(_instance.handle(), true);
    createDevicesAndQueues();
    createMemoryAllocator();

    vulkan::loadDeviceExtensions(_physicalDevice, _device.handle(), true);

    createFrameResources();

    _descriptorSetAllocator = std::make_unique<vulkan::DescriptorSetAllocator>();
    _descriptorSetAllocator->init(this);
    _cleanupManager.push([&](VkDevice)
    {
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

const vulkan::Surface& Engine::surface() const
{
    if (isOffscreen())
    {
        throw std::runtime_error("Engine::surface(): not allowed in offscreen application");
    }
    return _surface;
}

vulkan::Swapchain& Engine::swapchain()
{
    if (isOffscreen())
    {
        throw std::runtime_error("Engine::swapchain(): not allowed in offscreen application");
    }
    return _swapchain;
}

const vulkan::Swapchain& Engine::swapchain() const
{
    if (isOffscreen())
    {
        throw std::runtime_error("Engine::swapchain(): not allowed in offscreen application");
    }
    return _swapchain;
}

bool Engine::newFrame()
{
    if (isOffscreen())
    {
        return false;
    }
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
    if (isOffscreen())
    {
        _instance.create();
    }
    else
    {
        _instance.create(_windowPtr);
    }
}

void Engine::createSurface()
{
    _surface.create(_instance.handle(), _windowPtr);
}

void Engine::createDevicesAndQueues()
{
    if (isOffscreen())
    {
        _physicalDevice.choose(_instance.handle());
    }
    else
    {
        _physicalDevice.choose(_instance.handle(), _surface);
    }

    _device.create(_instance.handle(), _physicalDevice, isOffscreen());
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
    auto numImages = this->numImages();
    _frameResources.resize(numImages);
    for (uint32_t i = 0; i < numImages; ++i)
    {
        _frameResources[i].init(this, &_command);
    }
}

void Engine::cleanupFrameResources()
{
    for (uint32_t i = 0; i < numImages(); ++i)
    {
        _frameResources[i].cleanup();
    }
}

uint32_t Engine::currentFrameResourcesIndex() const
{
    return isOffscreen() ? 0 : _currentFrame % _frameResources.size();
}

uint32_t Engine::prevFrameResourcesIndex() const
{
    return isOffscreen() ? 0 : (_currentFrame - 1) % _frameResources.size();
}

vulkan::FrameResources& Engine::currentFrameResources() {
    return _frameResources[currentFrameResourcesIndex()];
}

const vulkan::FrameResources& Engine::currentFrameResources() const
{
    return _frameResources[currentFrameResourcesIndex()];
}

void Engine::iterateFrameResources(std::function<void(vulkan::FrameResources&)> cb)
{
    for (size_t i = 0; i < numImages(); ++i)
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
