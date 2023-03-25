
#ifndef bg2e_render_vulkan_vulkanapi_hpp
#define bg2e_render_vulkan_vulkanapi_hpp

#include <vulkan/vulkan.hpp>

#include <bg2e/export.hpp>
#include <bg2e/app/Window.hpp>
#include <bg2e/render/vulkan/ResourceDestroyManager.hpp>
#include <bg2e/render/vulkan/tools.hpp>
#include <bg2e/render/vulkan/ImmediateCommandBuffer.hpp>

#include <vector>
#include <memory>

namespace bg2e {
namespace render {
namespace vulkan {



class BG2E_EXPORT VulkanAPI {
public:
    void init(bool validationLayers, const std::string& appName, app::Window& window, uint32_t simultaneousFrames = 2);
    
    // Returns the swap chain image index or -1 if the framebuffer is invalid
    // (for example, after window resize)
    int32_t beginFrame();
    
    void endFrame(int32_t swapChainImageIndex);
    
    void destroy();
    
    VmaAllocator allocator() const { return _allocator; }

    vk::Instance instance() const { return _instance; }
    
    vk::SurfaceKHR surface() const { return _surface; }
    
    vk::RenderPass mainRenderPass() const { return _mainRenderPass; }
    
    uint32_t simultaneousFrames() const { return _simultaneousFrames; }
    
    uint32_t currentFrame() const { return _currentFrame; }
    
    vk::CommandBuffer commandBuffer() const { return _commandBuffers[_currentFrame]; }
    
    // Image index: the index returned by beginFrame
    vk::Framebuffer framebuffer(int32_t imageIndex) const { return _framebuffers[imageIndex]; }
    
    const SwapChainResources& swapchainResources() const { return _swapChain; }
    
    ResourceDestroyManager destroyManager;
    
    void setResized() { _framebufferResized = true; }
    
protected:
    bool _enableValidationLayers;
    std::string _appName;
    app::Window* _window;
    
    uint32_t _simultaneousFrames;
    uint32_t _currentFrame;
    
    VmaAllocator _allocator;

    vk::Instance _instance = nullptr;
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::SurfaceKHR _surface = nullptr;
    vk::PhysicalDevice _physicalDevice = nullptr;
    vk::Device _device = nullptr;
    vk::Queue _graphicsQueue = nullptr;
    vk::Queue _presentQueue = nullptr;
    SwapChainResources _swapChain;
    vk::RenderPass _mainRenderPass = nullptr;
    DepthResources _depthResources;
    std::vector<vk::Framebuffer> _framebuffers;
    vk::CommandPool _commandPool;
    std::vector<vk::CommandBuffer> _commandBuffers;
    std::vector<FrameSync> _frameSyncResources;
    bool _framebufferResized = false;
    
    std::unique_ptr<ImmediateCommandBuffer> _cmdExec;
    
    void getWindowSize(void* winImplPtr, int& width, int& height, bool fromFramebuffer);
    
    void recreateSwapChain();
};


}
}
}
#endif

