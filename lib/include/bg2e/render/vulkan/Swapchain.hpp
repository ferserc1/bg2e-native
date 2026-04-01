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
    
    inline const Image* msaaDepthImage() const { return _msaaDepthImage.get(); }
    
    inline VkFormat depthImageFormat() const { return _depthImage != nullptr ? _depthImage->format() : VK_FORMAT_UNDEFINED; }

    void screenshot(
        std::vector<uint8_t>& outData,
        uint32_t& outWidth,
        uint32_t& outHeight,
        uint32_t& outBpp
    );

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
    std::shared_ptr<Image> _msaaDepthImage;

    Engine* _engine = nullptr;
    
    void create(uint32_t, uint32_t);
};

}
}
}

