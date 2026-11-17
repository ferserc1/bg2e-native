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
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <stdexcept>
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
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.createRenderTarget", _diagnosticIdentity.value);
    _device = device;
    _metalDevice = dynamic_cast<metal::Device*>(device);

    _colorImage = std::make_unique<metal::Image>();
    _colorImage->buildTargetImage(_metalDevice, _size, _colorFormat, "offscreen-color-image");

    createDepthTarget(_size, _depthFormat);

    _frame = std::make_shared<metal::SurfaceFrame>();
    _frame->setColorImageRef(_colorImage.get());
    _frame->setDepthImage(_depthImage.get());
    ++_generation;
}

void OffscreenSurface::resize(const Size2D& size)
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.resize", _diagnosticIdentity.value);
    if (_device)
    {
        _device->waitIdle();
    }
    if (_frame)
    {
        bg2e_deep_trace("metal", "surface.slotWait.enter", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
        _frame->waitForSubmissions();
        bg2e_deep_trace("metal", "surface.slotWait.return", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
    }
    ++_generation;
    _frameAcquired = false;
    if (!size.width || !size.height)
    {
        _size = size;
        return;
    }
    _size = size;
    if (_colorImage)
    {
        _colorImage->resize(size);
    }
    resizeDepthTarget(size);
}

void OffscreenSurface::releaseRenderTarget()
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.releaseRenderTarget", _diagnosticIdentity.value);
    if (_device && _device->isValid())
    {
        _device->waitIdle();
    }
    if (_frame)
    {
        bg2e_deep_trace("metal", "surface.slotWait.enter", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
        _frame->waitForSubmissions();
        bg2e_deep_trace("metal", "surface.slotWait.return", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
    }
    _frameAcquired = false;
    _frame.reset();
    releaseDepthTarget();
    _colorImage.reset();
}

void OffscreenSurface::cleanup()
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.cleanup", _diagnosticIdentity.value);
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
    if (index == 0)
    {
        return _colorImage.get();
    }
    return nullptr;
}

gpu::Image* OffscreenSurface::depthImage() const
{
    return _depthImage.get();
}

std::shared_ptr<gpu::SurfaceFrame> OffscreenSurface::beginFrame()
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.beginFrame", _diagnosticIdentity.value);
    if (_frameAcquired)
    {
        throw std::logic_error("Offscreen surface already has an acquired frame");
    }
    if (!_size.width || !_size.height || !_frame)
    {
        return nullptr;
    }
    bg2e_deep_trace("metal", "surface.slotWait.enter", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
    _frame->waitForSubmissions();
    bg2e_deep_trace("metal", "surface.slotWait.return", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
    // Update the frame's color image reference (it may have been resized)
    _frame->setColorImageRef(_colorImage.get());
    _frameAcquired = true;
    return _frame;
}

void OffscreenSurface::present(gpu::CommandBuffer* cmd)
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.present", _diagnosticIdentity.value);
    auto* command = dynamic_cast<metal::CommandBuffer*>(cmd);
    if (!command || !_frame || !_frameAcquired)
    {
        throw std::logic_error("Invalid offscreen frame association");
    }
    command->associateFrame(_frame);
    command->_diagnosticSurface = _diagnosticIdentity.value;
    command->_diagnosticFrame = _frameCounter;
    command->_diagnosticSlot = currentFrameIndex();
    bg2e_deep_trace("metal", "frame.associate", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter
        << " slot=" << currentFrameIndex() << " command=" << command->_diagnosticIdentity.value);
}

void OffscreenSurface::endFrame(gpu::SurfaceFrame* frame)
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.endFrame", _diagnosticIdentity.value);
    if (!_frameAcquired || !_frame || frame != _frame.get() || !frame->hasSubmissions())
    {
        throw std::logic_error("Offscreen endFrame requires a submitted frame");
    }
    _frameAcquired = false;
    ++_frameCounter;
}

#else

OffscreenSurface::~OffscreenSurface() = default;

void OffscreenSurface::createRenderTarget(gpu::Device*, gpu::PhysicalDevice*)
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.createRenderTarget", _diagnosticIdentity.value);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void OffscreenSurface::resize(const Size2D&)
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.resize", _diagnosticIdentity.value);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void OffscreenSurface::releaseRenderTarget()
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.releaseRenderTarget", _diagnosticIdentity.value);
}

void OffscreenSurface::cleanup()
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.cleanup", _diagnosticIdentity.value);
}

bool OffscreenSurface::isValid() const
{
    return false;
}

uint32_t OffscreenSurface::imageCount() const
{
    return 0;
}

uint32_t OffscreenSurface::inFlightFrames() const
{
    return 0;
}

uint32_t OffscreenSurface::currentFrameIndex() const
{
    return 0;
}

gpu::Image* OffscreenSurface::colorImage(uint32_t) const
{
    return nullptr;
}

gpu::Image* OffscreenSurface::depthImage() const
{
    return nullptr;
}

std::shared_ptr<gpu::SurfaceFrame> OffscreenSurface::beginFrame()
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.beginFrame", _diagnosticIdentity.value);
    return nullptr;
}

void OffscreenSurface::present(gpu::CommandBuffer*)
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.present", _diagnosticIdentity.value);
}

void OffscreenSurface::endFrame(gpu::SurfaceFrame*)
{
    detail::DiagnosticScope diagnosticScope("metal", "OffscreenSurface.endFrame", _diagnosticIdentity.value);
    ++_frameCounter;
}

#endif

}
}
}
