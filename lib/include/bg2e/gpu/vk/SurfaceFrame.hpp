/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Image;

class SurfaceFrame : public gpu::SurfaceFrame {
public:
    gpu::Image* colorImage() const override { return _colorImage; }
    gpu::Image* depthImage() const override { return _depthImage; }
    bool        isValid()    const override { return _colorImage != nullptr; }

    void setColorImage(gpu::Image* img)        { _colorImage = img; }
    void setDepthImage(gpu::Image* img)        { _depthImage = img; }
    void setImageIndex(uint32_t index)         { _imageIndex = index; }
    void setSwapchain(VkSwapchainKHR sc)       { _swapchain = sc; }
    void setImageAvailable(VkSemaphore s)      { _imageAvailable = s; }
    void setRenderFinished(VkSemaphore s)      { _renderFinished = s; }
    void setInFlightFence(VkFence f)           { _inFlight = f; }

    uint32_t        imageIndex()     const { return _imageIndex; }
    VkSwapchainKHR  swapchain()      const { return _swapchain; }
    VkSemaphore     imageAvailable() const { return _imageAvailable; }
    VkSemaphore     renderFinished() const { return _renderFinished; }
    VkFence         inFlightFence()  const { return _inFlight; }

private:
    gpu::Image*    _colorImage     = nullptr;
    gpu::Image*    _depthImage     = nullptr;
    uint32_t       _imageIndex     = 0;
    VkSwapchainKHR _swapchain      = VK_NULL_HANDLE;
    VkSemaphore    _imageAvailable = VK_NULL_HANDLE;
    VkSemaphore    _renderFinished = VK_NULL_HANDLE;
    VkFence        _inFlight       = VK_NULL_HANDLE;
};

}
}
}