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

#include <bg2e/gpu/metal/OffscreenSurface.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/common.hpp>
#include <bg2e/gpu/Image.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

OffscreenSurface::~OffscreenSurface() = default;

void OffscreenSurface::createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* /*physicalDevice*/)
{
    _device = device;
    _metalDevice = dynamic_cast<metal::Device*>(device);

    _colorImage = std::make_unique<metal::Image>();
    _colorImage->buildTargetImage(_metalDevice, _size, _colorFormat);

    createDepthTarget(_size, _depthFormat);

    _frame = std::make_shared<metal::SurfaceFrame>();
    _frame->setColorImageRef(_colorImage.get());
    _frame->setDepthImage(_depthImage.get());
}

void OffscreenSurface::resize(const Size2D& size)
{
    _size = size;
    if (_colorImage) _colorImage->resize(size);
    resizeDepthTarget(size);
}

void OffscreenSurface::releaseRenderTarget()
{
    _frame.reset();
    releaseDepthTarget();
    _colorImage.reset();
}

void OffscreenSurface::cleanup()
{
    releaseRenderTarget();
}

bool OffscreenSurface::isValid() const
{
    return _colorImage && _colorImage->isValid();
}

uint32_t OffscreenSurface::imageCount() const { return 1; }
gpu::Image* OffscreenSurface::colorImage(uint32_t index) const
{
    if (index == 0) return _colorImage.get();
    return nullptr;
}
gpu::Image* OffscreenSurface::depthImage() const { return _depthImage.get(); }

std::shared_ptr<gpu::SurfaceFrame> OffscreenSurface::beginFrame()
{
    // Update the frame's color image reference (it may have been resized)
    _frame->setColorImageRef(_colorImage.get());
    return _frame;
}

void OffscreenSurface::present(gpu::CommandBuffer*)
{
    // No-op for offscreen
}

void OffscreenSurface::endFrame(gpu::SurfaceFrame*)
{
    // No-op for offscreen
}

#else

OffscreenSurface::~OffscreenSurface() = default;

void OffscreenSurface::createRenderTarget(gpu::Device*, gpu::PhysicalDevice*)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void OffscreenSurface::resize(const Size2D&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void OffscreenSurface::releaseRenderTarget() {}
void OffscreenSurface::cleanup() {}
bool OffscreenSurface::isValid() const { return false; }

uint32_t OffscreenSurface::imageCount() const { return 0; }
gpu::Image* OffscreenSurface::colorImage(uint32_t) const { return nullptr; }
gpu::Image* OffscreenSurface::depthImage() const { return nullptr; }

std::shared_ptr<gpu::SurfaceFrame> OffscreenSurface::beginFrame() { return nullptr; }
void OffscreenSurface::present(gpu::CommandBuffer*) {}
void OffscreenSurface::endFrame(gpu::SurfaceFrame*) {}

#endif

}
}
}
