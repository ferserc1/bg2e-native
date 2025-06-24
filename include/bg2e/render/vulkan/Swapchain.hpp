#pragma once

#include <vulkan/vulkan.h>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <vector>
#include <memory>

namespace bg2e {
namespace render {

class Engine;

namespace vulkan {

class BG2E_API Swapchain {
public:
    
    void init(
        Engine * engine,
        uint32_t width,
        uint32_t height
    );
    void resize(uint32_t width, uint32_t height);
    void cleanup();
    
    inline VkSwapchainKHR handle() const { return _swapchain; }
    inline VkFormat imageFormat() const { return _imageFormat; }
    inline const std::vector<VkImage>& images() const { return _images; }
    inline const std::vector<VkImageView>& imageViews() const { return _imageViews; }
    inline const VkExtent2D& extent() const { return _extent; }
    inline VkImage image(uint32_t index) const { return _images[index]; }
    inline VkImageView imageView(uint32_t index) const { return _imageViews[index]; }
    inline VkSampleCountFlagBits sampleCount() const { return _msaaSampleCount; }

    // Return the image, imageView, extent and format, wrapped into
    // a vmke::core::Image object.
    inline const Image* colorImage(uint32_t index) const {
        return _msaaImages[index].get();
    }
    
    inline const Image* msaaResolveImage(uint32_t index) const {
        return _colorImages[index];
    }
    
    inline const Image* depthImage() const { return _depthImage.get(); }
    
    inline const VkFormat depthImageFormat() const { return _depthImage != nullptr ? _depthImage->format() : VK_FORMAT_UNDEFINED; }

protected:
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkFormat _imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkSampleCountFlagBits _msaaSampleCount = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkImage> _images;
    std::vector<VkImageView> _imageViews;
    VkExtent2D _extent;

    std::vector<std::shared_ptr<Image>> _msaaImages;
    std::vector<Image *> _colorImages;
    std::shared_ptr<Image> _depthImage;

    Engine* _engine = nullptr;
    
    void create(uint32_t, uint32_t);
};

}
}
}

