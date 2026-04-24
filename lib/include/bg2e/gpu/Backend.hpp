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

#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/gpu/Surface.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Swapchain.hpp>
#include <bg2e/gpu/Command.hpp>
#include <bg2e/gpu/CleanupManager.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/Image.hpp>

#include <memory>
#include <cstdint>

namespace bg2e {
namespace gpu {

class BG2E_API Backend {
public:
    virtual ~Backend() = default;

    // Core lifecycle
    virtual void init(SDL_Window* window) = 0;
    virtual void cleanup() = 0;

    // Accessors (return abstract interfaces, not Vulkan handles)
    virtual Instance* instance() = 0;
    [[nodiscard]] virtual const Instance* instance() const = 0;
    virtual PhysicalDevice* physicalDevice() = 0;
    [[nodiscard]]virtual const PhysicalDevice* physicalDevice() const = 0;
    virtual Surface* surface() = 0;
    [[nodiscard]]virtual const Surface* surface() const = 0;
    virtual Device* device() = 0;
    [[nodiscard]]virtual const Device* device() const = 0;
    virtual Swapchain* swapchain() = 0;
    [[nodiscard]]virtual const Swapchain* swapchain() const = 0;
    virtual Command* command() = 0;
    [[nodiscard]]virtual const Command* command() const = 0;

    // Memory allocation (abstraction over VMA)
    virtual Buffer* createBuffer(size_t size, uint32_t usageFlags, uint32_t memoryUsage) = 0;
    virtual void destroyBuffer(Buffer* buffer) = 0;

    // Image creation
    virtual Image* createImage(uint32_t format, uint32_t width, uint32_t height, 
                               uint32_t usage, uint32_t aspectFlags,
                               uint32_t arrayLayers, bool useMipmaps,
                               uint32_t maxMipmapLevels, uint32_t samples) = 0;
    virtual Image* createImageFromData(const uint8_t* data, size_t dataSize,
                                       uint32_t width, uint32_t height, uint32_t bpp,
                                       uint32_t format, uint32_t usage) = 0;
    virtual void destroyImage(Image* image) = 0;

    // Frame management
    [[nodiscard]]virtual uint32_t currentFrame() const = 0;
    virtual void nextFrame() = 0;
    [[nodiscard]]virtual uint32_t currentFrameResourcesIndex() const = 0;
    [[nodiscard]]virtual uint32_t prevFrameResourcesIndex() const = 0;

    // Query
    [[nodiscard]] virtual bool rayQuerySupported() const = 0;
    [[nodiscard]] virtual bool rayTracingPipelineSupported() const = 0;
    virtual void updateSwapchainSize() = 0;

    // Frame advance (returns true if swapchain was resized)
    virtual bool newFrame() = 0;

};

}
}
