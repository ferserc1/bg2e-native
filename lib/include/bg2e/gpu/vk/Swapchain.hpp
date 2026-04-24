/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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

#include <bg2e/gpu/Swapchain.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <bg2e/gpu/vk/Image.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Swapchain : public gpu::Swapchain {
public:
    void init(void* engine, uint32_t width, uint32_t height) override;
    void resize(uint32_t width, uint32_t height) override;
    void cleanup() override;

    uint32_t width() const override;
    uint32_t height() const override;

    bool isValid() const override;
    
    Image* colorImage(uint32_t index) override;
    const Image* colorImage(uint32_t index) const override;
    Image* depthImage() override;
    const Image* depthImage() const override;

    inline VkSwapchainKHR handle() const { return _swapchain; }

private:
    VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
    VkFormat _imageFormat = VK_FORMAT_UNDEFINED;
    uint32_t _width = 0;
    uint32_t _height = 0;
    VkSampleCountFlagBits _msaaSampleCount = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkImage> _images;
    std::vector<VkImageView> _imageViews;
    std::vector<VkSemaphore> _renderSemaphores;
};

}
}
}
