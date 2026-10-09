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

#include <bg2e/gpu/vk/OffscreenSurface.hpp>
#include <bg2e/gpu/vk/Device.hpp>
#include <bg2e/gpu/vk/CommandBuffer.hpp>
#include <stdexcept>
#include <bg2e/gpu/vk/Image.hpp>
#include <bg2e/gpu/Image.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

OffscreenSurface::~OffscreenSurface() = default;

void OffscreenSurface::createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* /*physicalDevice*/)
{
    _device = device;
    _vkDevice = dynamic_cast<vk::Device*>(device);

    _colorImage = std::make_unique<vk::Image>();
    _colorImage->buildTargetImage(_vkDevice, _size, _colorFormat);

    createDepthTarget(_size, _depthFormat);

    _frame = std::make_shared<vk::SurfaceFrame>();
    _frame->setColorImage(_colorImage.get());
    _frame->setDepthImage(_depthImage.get());
    ++_generation;
}

void OffscreenSurface::resize(const Size2D& size)
{
    if (_device) _device->waitIdle();
    if (_frame) _frame->waitForSubmissions();
    ++_generation;
    _frameAcquired = false;
    if (!size.width || !size.height) { _size = size; return; }
    _size = size;
    if (_colorImage)
    {
        _colorImage->resize(size);
    }
    resizeDepthTarget(size);
}

void OffscreenSurface::releaseRenderTarget()
{
    if (_device && _device->isValid()) _device->waitIdle();
    if (_frame) _frame->waitForSubmissions();
    _frameAcquired = false;
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

uint32_t OffscreenSurface::imageCount() const
{
    return 1;
}

uint32_t OffscreenSurface::inFlightFrames() const
{
    return 1;
}

uint32_t OffscreenSurface::currentFrameIndex() const
{
    return 0;
}

gpu::Image* OffscreenSurface::colorImage(uint32_t index) const
{
    if (index == 0) return _colorImage.get();
    return nullptr;
}

gpu::Image* OffscreenSurface::depthImage() const
{
    return _depthImage.get();
}

std::shared_ptr<gpu::SurfaceFrame> OffscreenSurface::beginFrame()
{
    if (_frameAcquired) throw std::logic_error("Offscreen surface already has an acquired frame");
    if (!_size.width || !_size.height || !_frame) return nullptr;
    _frame->waitForSubmissions();
    _frameAcquired = true;
    return _frame;
}

void OffscreenSurface::present(gpu::CommandBuffer* cmd)
{
    auto* command = dynamic_cast<vk::CommandBuffer*>(cmd);
    if (!command || !_frame || !_frameAcquired) throw std::logic_error("Invalid offscreen frame association");
    command->associateFrame(_frame);
}

void OffscreenSurface::endFrame(gpu::SurfaceFrame* frame)
{
    if (!_frameAcquired || !_frame || frame != _frame.get() || !frame->hasSubmissions())
        throw std::logic_error("Offscreen endFrame requires a submitted frame");
    _frameAcquired = false;
    ++_frameCounter;
}

}
}
}
