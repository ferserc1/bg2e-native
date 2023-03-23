
#ifndef bg2e_render_vulkan_vulkanapi_hpp
#define bg2e_render_vulkan_vulkanapi_hpp

#include <vulkan/vulkan.hpp>

#include <bg2e/export.hpp>
#include <bg2e/app/Window.hpp>
#include <bg2e/render/vulkan/ResourceDestroyManager.hpp>
#include <bg2e/render/vulkan/tools.hpp>

#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_EXPORT VulkanAPI {
public:
    void init(bool validationLayers, const std::string& appName, app::Window& window);
    
    void destroy();
    
    vk::Instance instance() const { return _instance; }
    
    vk::SurfaceKHR surface() const { return _surface; }
    
    ResourceDestroyManager destroyManager;
    
protected:
    bool _enableValidationLayers;
    std::string _appName;
    
    vk::Instance _instance = nullptr;
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::SurfaceKHR _surface = nullptr;
    vk::PhysicalDevice _physicalDevice = nullptr;
    vk::Device _device = nullptr;
    vk::Queue _graphicsQueue = nullptr;
    vk::Queue _presentQueue = nullptr;
    SwapChainResources _swapChain;
    
};


}
}
}
#endif

