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

#include <bg2e/gpu/Backend.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <bg2e/gpu/vk/Instance.hpp>
#include <bg2e/gpu/vk/Surface.hpp>
#include <bg2e/gpu/vk/PhysicalDevice.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/Swapchain.hpp>
#include <bg2e/gpu/vk/Command.hpp>

#include <memory>

namespace bg2e {
namespace gpu {
namespace vk {


class Backend : public gpu::Backend {
public:
    void init(SDL_Window* window) override;
    void setEngine(void* engine);
    void cleanup() override;

    gpu::Instance* instance() override { return _instance.get(); }
    const gpu::Instance* instance() const override { return _instance.get(); }
    gpu::PhysicalDevice* physicalDevice() override { return _physicalDevice.get(); }
    const gpu::PhysicalDevice* physicalDevice() const override { return _physicalDevice.get(); }
    gpu::Surface* surface() override { return _surface.get(); }
    const gpu::Surface* surface() const override { return _surface.get(); }
    gpu::Device* device() override { return _device.get(); }
    const gpu::Device* device() const override { return _device.get(); }
    gpu::Swapchain* swapchain() override { return _swapchain.get(); }
    const gpu::Swapchain* swapchain() const override { return _swapchain.get(); }
    gpu::Command* command() override { return _command.get(); }
    const gpu::Command* command() const override { return _command.get(); }

    Buffer* createBuffer(size_t size, uint32_t usageFlags, uint32_t memoryUsage) override;
    void destroyBuffer(Buffer* buffer) override;

    Image* createImage(uint32_t format, uint32_t width, uint32_t height,
                       uint32_t usage, uint32_t aspectFlags,
                       uint32_t arrayLayers = 1, bool useMipmaps = false, 
                       uint32_t maxMipmapLevels = 1, uint32_t samples = 0) override;
    Image* createImageFromData(const uint8_t* data, size_t dataSize,
                               uint32_t width, uint32_t height, uint32_t bpp,
                               uint32_t format, uint32_t usage) override;
    void destroyImage(gpu::Image* image) override;

    uint32_t currentFrame() const override { return _currentFrame; }
    void nextFrame() override { ++_currentFrame; }

    [[nodiscard]] bool rayQuerySupported() const override;
    [[nodiscard]] bool rayTracingPipelineSupported() const override;
    void updateSwapchainSize() override { _resizeRequested = true; }
    bool newFrame() override;

private:
    void* _engine = nullptr;

    std::unique_ptr<vk::Instance> _instance;
    std::unique_ptr<vk::Surface> _surface;
    std::unique_ptr<vk::PhysicalDevice> _physicalDevice;
    std::unique_ptr<vk::Device> _device;
    std::unique_ptr<vk::Swapchain> _swapchain;
    std::unique_ptr<vk::Command> _command;

    uint32_t _currentFrame = 0;
    bool _resizeRequested = false;

    VmaAllocator _allocator = VK_NULL_HANDLE;
};

}
}
}
