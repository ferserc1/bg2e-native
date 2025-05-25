
#include <bg2e/render/Vulkan.hpp>
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

void Vulkan::init(SDL_Window* windowPtr)
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
        _descriptorSetAllocator->clearDescriptors();
        _descriptorSetAllocator->destroy();
    });
}

void Vulkan::cleanup()
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

bool Vulkan::newFrame()
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

void Vulkan::createInstance()
{
    _instance.create(_windowPtr);
}

void Vulkan::createSurface()
{
    _surface.create(_instance, _windowPtr);
}

void Vulkan::createDevicesAndQueues()
{
	_physicalDevice.choose(_instance, _surface);
    _device.create(_instance, _physicalDevice, _surface);
    _command.init(this);
}

void Vulkan::createMemoryAllocator()
{
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physicalDevice.handle();
    allocInfo.device = _device.handle();
    allocInfo.instance = _instance.handle();
    allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocInfo, &_allocator);
}

void Vulkan::createFrameResources()
{
    for (int i = 0; i < vulkan::FRAME_OVERLAP; ++i)
    {
        _frameResources[i].init(_device, &_command);
    }
}

void Vulkan::cleanupFrameResources()
{
    for (int i = 0; i < vulkan::FRAME_OVERLAP; ++i)
    {
        _frameResources[i].cleanup();
    }
}

void Vulkan::iterateFrameResources(std::function<void(vulkan::FrameResources&)> cb)
{
    for (auto i = 0; i < vulkan::FRAME_OVERLAP; ++i)
    {
        cb(_frameResources[i]);
    }
}

void Vulkan::destroyBuffer(VkBuffer buffer, VmaAllocation allocation)
{
    vmaDestroyBuffer(_allocator, buffer, allocation);
}

}
}
