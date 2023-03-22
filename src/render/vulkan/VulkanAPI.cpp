
#include <bg2e/render/vulkan/VulkanAPI.hpp>

#include <bg2e/render/vulkan/tools.hpp>

#include <GLFW/glfw3.h>


namespace bg2e {
namespace render {
namespace vulkan {

void VulkanAPI::init(bool validationLayers, const std::string& appName, app::Window& window)
{
    _enableValidationLayers = validationLayers;
    
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
    
    destroyManager.push_function([=]() {
        instance().destroySurfaceKHR(surface, nullptr);
    });
    
    _physicalDevice = pickPhysicalDevice(_instance, _surface);
    QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);
    _device = createDevice(_instance, _physicalDevice, _surface, _enableValidationLayers);
    _presentQueue = _device.getQueue(indices.presentFamily.value(), 0);
    _graphicsQueue = _device.getQueue(indices.graphicsFamily.value(), 0);
    
    destroyManager.push_function([=]() {
        _device.destroy();
    });
}


void VulkanAPI::destroy()
{
    destroyManager.flush();
}




}
}
}
