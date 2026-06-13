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

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>

#include <memory>

namespace bg2e {
namespace gpu {

class Device;
class PhysicalDevice;
class Image;
class Instance;
class SurfaceFrame;
class CommandBuffer;

namespace vk    { class Backend; class Device; class Surface; }
namespace metal { class Backend; class Device; class Surface; }

class BG2E_API Surface {
public:
    virtual ~Surface() = default;

    virtual bool isOffscreen() const = 0;
    virtual bool isValid() const = 0;

    const Size2D& size() const { return _size; }
    void setSize(const Size2D& s) { _size = s; }

    virtual uint32_t width() const { return _size.width; }
    virtual uint32_t height() const { return _size.height; }

    PixelFormat colorFormat() const { return _colorFormat; }
    PixelFormat depthFormat() const { return _depthFormat; }
    void setColorFormat(PixelFormat f) { _colorFormat = f; }
    void setDepthFormat(PixelFormat f) { _depthFormat = f; }

    virtual void resize(const Size2D& size) = 0;
    virtual void releaseRenderTarget() = 0;

    virtual uint32_t    imageCount() const = 0;
    virtual uint32_t    currentFrameIndex() const = 0;
    virtual gpu::Image* colorImage(uint32_t index) const = 0;
    virtual gpu::Image* depthImage() const = 0;

    // Frame lifecycle
    virtual std::shared_ptr<SurfaceFrame> beginFrame() = 0;
    virtual void present(gpu::CommandBuffer* cmd) = 0;
    virtual void endFrame(SurfaceFrame* frame) = 0;

    virtual void cleanup() = 0;

protected:
    // Surface creation lifecycle. These are not part of the public API:
    //  - create() is invoked by the Backend factory (createWindowSurface /
    //    createOffscreenSurface), which owns the Instance.
    //  - createRenderTarget() is invoked by the Device when it is created.
    // Both Backend and Device are granted access through the friendship
    // declarations below. The default create() is a no-op (offscreen
    // surfaces do not need a window-system surface).
    virtual void create(Instance* instance) {}
    virtual void createRenderTarget(Device* device, PhysicalDevice* physicalDevice) = 0;

    Size2D          _size;
    PixelFormat     _colorFormat = PixelFormat::Undefined;
    PixelFormat     _depthFormat = PixelFormat::Undefined;
    Device*         _device         = nullptr;
    PhysicalDevice* _physicalDevice = nullptr;

    friend class vk::Backend;
    friend class vk::Device;
    friend class vk::Surface;
    friend class metal::Backend;
    friend class metal::Device;
    friend class metal::Surface;
};

}
}
