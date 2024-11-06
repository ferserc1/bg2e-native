
#include <bg2e/render/Vulkan.hpp>

#include <SDL2/SDL.h>
#include <SDL_vulkan.h>

namespace bg2e {
namespace render {

void Vulkan::init(SDL_Window* windowPtr)
{
    _windowPtr = windowPtr;

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(_windowPtr, &width, &height);

    createInstance();
    createSurface();
    createDevicesAndQueues();
    createMemoryAllocator();
    
    _swapchain.init(this, uint32_t(width), uint32_t(height));
    
    createFrameResources();
}

void Vulkan::cleanup()
{
    vkDeviceWaitIdle(_device);

    _cleanupManager.flush(_device);

    cleanupFrameResources();

    _swapchain.cleanup();

    vmaDestroyAllocator(_allocator);

    vkDestroyDevice(_device, nullptr);

	bg2e::render::destroySurface(_instance, _surface, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

bool Vulkan::newFrame()
{
    if (_resizeRequested)
    {
        vkDeviceWaitIdle(_device);

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
#if BG2E_IS_MAC
    // Due to a bug in vk-bootstrap, it is not possible to load the dynamic rendering extension on Mac correctly.
    // There is a workaround that consists in activating the feature through vulkan 1.3, but as MoltenVK only
    // supports version 1.2, if the validation layers are activated this gives an error.
    // To fix this the options are to not use vk-bootstrap, wait for them to fix the bug or wait for MoltenVK
    // to support version 1.3. As the macOS platform is not a priority, we will leave the validation layers
    // disabled for the time being.
    _debugLayers = false;
#endif
    auto instanceBuilder = bg2e::render::createInstanceBuilder("bg2 engine")
        .request_validation_layers(_debugLayers)
        .require_api_version(VK_VERSION_1_3)
        .use_default_debug_messenger()
        .build();

    _vkbInstance = instanceBuilder.value();
    _instance = _vkbInstance.instance;
    _debugMessenger = _vkbInstance.debug_messenger;
}

void Vulkan::createSurface()
{
    SDL_Vulkan_CreateSurface(_windowPtr, _instance, &_surface);
}

void Vulkan::createDevicesAndQueues()
{
    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    VkPhysicalDeviceVulkan11Features features11 = {};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.multiview = true;

    VkPhysicalDeviceFeatures features = {};
    features.samplerAnisotropy = true;

    vkb::PhysicalDeviceSelector selector{ _vkbInstance };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 2)
        .set_required_features_13(features13)
        .set_required_features_12(features12)
        .set_required_features_11(features11)
        .set_required_features(features)
        .set_surface(_surface)
        .select()
        .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    _physicalDevice = physicalDevice.physical_device;
    _device = vkbDevice.device;

    _command.init(this, &vkbDevice);
}

void Vulkan::createMemoryAllocator()
{
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physicalDevice;
    allocInfo.device = _device;
    allocInfo.instance = _instance;
    allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocInfo, &_allocator);
}

void Vulkan::createFrameResources()
{
    for (int i = 0; i < FRAME_OVERLAP; ++i)
    {
        _frameResources[i].init(_device, &_command);
    }
}

void Vulkan::cleanupFrameResources()
{
    for (int i = 0; i < FRAME_OVERLAP; ++i)
    {
        _frameResources[i].cleanup();
    }
}

void Vulkan::iterateFrameResources(std::function<void(FrameResources&)> cb)
{
    for (auto i = 0; i < FRAME_OVERLAP; ++i)
    {
        cb(_frameResources[i]);
    }
}

}
}
