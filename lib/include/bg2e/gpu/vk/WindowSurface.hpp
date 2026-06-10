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

#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/vk/Surface.hpp>
#include <bg2e/gpu/vk/common.hpp>
#include <bg2e/gpu/vk/SurfaceFrame.hpp>

#include <SDL2/SDL.h>

#include <memory>
#include <vector>

namespace bg2e {
namespace gpu {
namespace vk {

class Image;

class WindowSurface : public gpu::WindowSurface, public Surface {
public:
    void cleanup() override;

    uint32_t width() const override;
    uint32_t height() const override;

    bool isValid() const override;

    VkSurfaceKHR handle() const;
    SDL_Window* sdlWindow() const;

    void resize(const Size2D& size) override;
    void releaseRenderTarget() override;

    uint32_t    imageCount() const override;
    gpu::Image* colorImage(uint32_t index) const override;
    gpu::Image* depthImage() const override;

    std::shared_ptr<gpu::SurfaceFrame> beginFrame() override;
    void present(gpu::CommandBuffer* cmd) override;
    void endFrame(gpu::SurfaceFrame* frame) override;

    VkSwapchainKHR swapchain() const;

protected:
    void create(gpu::Instance* instance) override;
    void createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* physicalDevice) override;

private:
    VkSurfaceKHR _surface{VK_NULL_HANDLE};
    VkInstance _vkInstance{VK_NULL_HANDLE};
    SDL_Window* _window{nullptr};

    VkSwapchainKHR _swapchain{VK_NULL_HANDLE};
    std::vector<std::unique_ptr<vk::Image>> _colorImages;

    // Number of frames that can be processed concurrently. Determined at
    // render-target creation time from the actual swapchain image count, so the
    // CPU never reuses per-frame resources while the GPU is still consuming them.
    uint32_t _framesInFlight = 0;
    uint32_t _currentFrame   = 0;

    // Per frame-in-flight (indexed by _currentFrame).
    std::vector<VkSemaphore> _imageAvailable;
    std::vector<VkFence>     _inFlight;
    std::vector<std::shared_ptr<vk::SurfaceFrame>> _frames;

    // Per swapchain image (indexed by the acquired image index). renderFinished
    // must be per-image because vkQueuePresentKHR waits on it: a semaphore cannot
    // be reused until its presentation has completed.
    std::vector<VkSemaphore> _renderFinished;

    // Tracks which in-flight fence (if any) is currently rendering into each
    // swapchain image, so an image acquired out of order is not reused while a
    // previous frame is still writing to it. Non-owning aliases of _inFlight.
    std::vector<VkFence> _imagesInFlight;
};

}
}
}
