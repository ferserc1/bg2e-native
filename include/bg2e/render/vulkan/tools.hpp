
#ifndef bg2e_render_vulkan_tools_hpp
#define bg2e_render_vulkan_tools_hpp

#include <vulkan/vulkan.hpp>

#include <vma/vk_mem_alloc.h>

#include <bg2e/app/Window.hpp>

#include <vector>
#include <string>
#include <optional>

namespace bg2e {
namespace render {
namespace vulkan {

struct AllocatedBuffer {
    vk::Buffer buffer = nullptr;
    VmaAllocation allocation;
};

struct AllocatedImage {
    vk::Image image = nullptr;
    VmaAllocation allocation;
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    
    inline bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct SwapChainResources
{
    vk::SwapchainKHR swapchain = nullptr;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;
    vk::Extent2D extent { 0, 0 };
    vk::Format imageFormat;
    
    inline bool isValid() const {
        return images.size() > 0 && imageViews.size() > 0;
    }
};

struct DepthResources
{
    AllocatedImage image;
    vk::ImageView view;
};

bool checkValidationLayerSupport();

bool checkDeviceExtensions(vk::PhysicalDevice device);

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

std::vector<const char*> getRequiredExtensions(bool validationLayersSupport);

vk::Instance createInstance(vk::ApplicationInfo& appInfo, bool validationLayersSupport);

VkDebugUtilsMessengerEXT setupDebugMessenger(vk::Instance instance);

void destroyDebugMessenger(vk::Instance instance, VkDebugUtilsMessengerEXT debugMessenger);

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);

bool isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface);

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

vk::PresentModeKHR chooseSwapPresentFormat(const std::vector<vk::PresentModeKHR>& availablePresentModes);

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, app::Window&);

vk::PhysicalDevice pickPhysicalDevice(vk::Instance, vk::SurfaceKHR);

vk::Device createDevice(vk::Instance, vk::PhysicalDevice, vk::SurfaceKHR, bool enableValidationLayers);

void createSwapChain(vk::Instance, vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, app::Window&, SwapChainResources&);

void destroySwapChain(vk::Device, SwapChainResources&);

vk::Format findSupportedFormat(vk::PhysicalDevice device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

vk::Format findDepthFormat(vk::PhysicalDevice device);

vk::RenderPass createBasicDepthRenderPass(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Format colorFormat);

AllocatedImage createImage(VmaAllocator allocator, vk::Device device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryFlags);

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);

DepthResources createDepthResources(VmaAllocator allocator, vk::PhysicalDevice physicalDevice, vk::Device device, const SwapChainResources& swapChainData);

void destroyDepthResources(VmaAllocator allocator, vk::Device device, DepthResources& res);

}
}
}

#endif

