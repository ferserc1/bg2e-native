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

#include <bg2e/gpu/metal/WindowSurface.hpp>
#include <bg2e/base/CpuProfile.hpp>
#include <bg2e/gpu/metal/common.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/SurfaceFrame.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/Image.hpp>

#include <stdexcept>

#if BG2E_IS_MAC
#include <SDL2/SDL_metal.h>
#endif

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

void WindowSurface::create(gpu::Instance* instance)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.create", _diagnosticIdentity.value);
    SDL_Window* window = instance->window();
    if (!window)
    {
        throw std::runtime_error("metal::WindowSurface::create: instance has no window");
    }

    _metalView = SDL_Metal_CreateView(window);
    if (!_metalView)
    {
        throw std::runtime_error("metal::WindowSurface::create: SDL_Metal_CreateView failed");
    }

    _layer = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(_metalView));

    int w, h;
    SDL_Metal_GetDrawableSize(window, &w, &h);
    _size = Size2D{static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
}

void WindowSurface::cleanup()
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.cleanup", _diagnosticIdentity.value);
    releaseRenderTarget();
    if (_metalView)
    {
        SDL_Metal_DestroyView(_metalView);
        _metalView = nullptr;
    }
    _layer = nullptr;
    _size = Size2D{};
}

uint32_t WindowSurface::width() const
{
    return _size.width;
}

uint32_t WindowSurface::height() const
{
    return _size.height;
}

bool WindowSurface::isValid() const
{
    return _layer != nullptr;
}

void WindowSurface::createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* /*physicalDevice*/)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.createRenderTarget", _diagnosticIdentity.value);
    _device = device;
    _metalDevice = dynamic_cast<metal::Device*>(device);

    _layer->setDevice(_metalDevice->handle());
    _layer->setPixelFormat(toMetalPixelFormat(_colorFormat));
    _layer->setDrawableSize(CGSize{double(_size.width), double(_size.height)});
    _layer->setMaximumDrawableCount(3);
    _layer->setFramebufferOnly(false);
    _imageCount = 3;

    if (_size.width && _size.height)
    {
        createDepthTarget(_size, _depthFormat);
    }
    ++_generation;
}

void WindowSurface::resize(const Size2D& size)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.resize", _diagnosticIdentity.value);
    if (_device)
    {
        _device->waitIdle();
    }
    for (auto& frame : _frames)
    {
        if (frame)
        {
            bg2e_deep_trace("metal", "surface.slotWait.enter", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
            frame->waitForSubmissions();
            bg2e_deep_trace("metal", "surface.slotWait.return", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
        }
        frame.reset();
    }
    _currentFrame.reset();
    _size = size;
    _layer->setDrawableSize(CGSize{double(size.width), double(size.height)});
    if (size.width && size.height)
    {
        if (_depthImage)
        {
            resizeDepthTarget(size);
        }
        else
        {
            createDepthTarget(size, _depthFormat);
        }
    }
    ++_generation;
}

void WindowSurface::releaseRenderTarget()
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.releaseRenderTarget", _diagnosticIdentity.value);
    if (_device && _device->isValid())
    {
        _device->waitIdle();
    }
    for (auto& frame : _frames)
    {
        if (frame)
        {
            bg2e_deep_trace("metal", "surface.slotWait.enter", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
            frame->waitForSubmissions();
            bg2e_deep_trace("metal", "surface.slotWait.return", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
        }
        frame.reset();
    }
    _currentFrame.reset();
    releaseDepthTarget();
}

uint32_t WindowSurface::imageCount() const
{
    return _imageCount;
}

uint32_t WindowSurface::inFlightFrames() const
{
    return 2;
}

uint32_t WindowSurface::currentFrameIndex() const
{
    return _currentFrameIndex;
}

gpu::Image* WindowSurface::colorImage(uint32_t /*index*/) const
{
    return nullptr;
}

gpu::Image* WindowSurface::depthImage() const
{
    return _depthImage.get();
}

std::shared_ptr<gpu::SurfaceFrame> WindowSurface::beginFrame()
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.beginFrame", _diagnosticIdentity.value);
    if (_currentFrame)
    {
        throw std::logic_error("Metal surface already has an acquired frame");
    }
    if (!_size.width || !_size.height || !_layer)
    {
        return nullptr;
    }
    auto& slot = _frames[_currentFrameIndex];
    if (slot)
    {
        base::CpuProfileScope profile("metal.surface.waitSlotSubmissions");
        bg2e_deep_trace("metal", "surface.slotWait.enter", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
        slot->waitForSubmissions();
        bg2e_deep_trace("metal", "surface.slotWait.return", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << currentFrameIndex());
    }
    slot.reset();
    base::CpuProfileScope profile("metal.surface.nextDrawable");
    bg2e_deep_trace("metal", "surface.acquire.enter", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << _currentFrameIndex);
    auto* drawable = _layer->nextDrawable();
    bg2e_deep_trace("metal", "surface.acquire.return", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter << " slot=" << _currentFrameIndex << " drawable=" << (drawable != nullptr));
    profile.next("metal.surface.wrapDrawable");
    if (!drawable)
    {
        return nullptr;
    }
    auto frame = std::make_shared<metal::SurfaceFrame>();
    frame->setDrawable(drawable);
    auto color = std::make_unique<metal::Image>();
    color->initFromDrawableTexture(_metalDevice, drawable->texture(), _colorFormat, _size);
    frame->setColorImage(std::move(color));
    frame->setDepthImage(_depthImage.get());
    slot = frame;
    _currentFrame = frame;
    return frame;
}

void WindowSurface::present(gpu::CommandBuffer* cmd)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.present", _diagnosticIdentity.value);
    auto* command = dynamic_cast<metal::CommandBuffer*>(cmd);
    if (!command || !_currentFrame || !_currentFrame->isValid())
    {
        throw std::logic_error("Metal presentation requires a valid acquired frame and command buffer");
    }
    command->associateFrame(_currentFrame);
    command->_diagnosticSurface = _diagnosticIdentity.value;
    command->_diagnosticFrame = _frameCounter;
    command->_diagnosticSlot = currentFrameIndex();
    bg2e_deep_trace("metal", "frame.associate", "surface=" << _diagnosticIdentity.value << " frame=" << _frameCounter
        << " slot=" << currentFrameIndex() << " command=" << command->_diagnosticIdentity.value);
    command->handle()->presentDrawable(_currentFrame->drawable());
}

void WindowSurface::endFrame(gpu::SurfaceFrame* frame)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.endFrame", _diagnosticIdentity.value);
    if (!_currentFrame || frame != _currentFrame.get() || !frame->hasSubmissions())
    {
        throw std::logic_error("Metal endFrame requires a submitted acquired frame");
    }
    _currentFrame.reset();
    _currentFrameIndex = (_currentFrameIndex + 1) % 2;
    ++_frameCounter;
}

#else

void WindowSurface::create(gpu::Instance*)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.create", _diagnosticIdentity.value);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void WindowSurface::cleanup()
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.cleanup", _diagnosticIdentity.value);
}

uint32_t WindowSurface::width() const
{
    return 0;
}

uint32_t WindowSurface::height() const
{
    return 0;
}

bool WindowSurface::isValid() const
{
    return false;
}

void WindowSurface::createRenderTarget(gpu::Device*, gpu::PhysicalDevice*)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.createRenderTarget", _diagnosticIdentity.value);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void WindowSurface::resize(const Size2D&)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.resize", _diagnosticIdentity.value);
    throw std::runtime_error("Metal backend is not available on this platform");
}

void WindowSurface::releaseRenderTarget()
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.releaseRenderTarget", _diagnosticIdentity.value);
}

uint32_t WindowSurface::imageCount() const
{
    return 0;
}

uint32_t WindowSurface::inFlightFrames() const
{
    return 0;
}

uint32_t WindowSurface::currentFrameIndex() const
{
    return 0;
}

gpu::Image* WindowSurface::colorImage(uint32_t) const
{
    return nullptr;
}

gpu::Image* WindowSurface::depthImage() const
{
    return nullptr;
}

std::shared_ptr<gpu::SurfaceFrame> WindowSurface::beginFrame()
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.beginFrame", _diagnosticIdentity.value);
    return nullptr;
}

void WindowSurface::present(gpu::CommandBuffer*)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.present", _diagnosticIdentity.value);
}

void WindowSurface::endFrame(gpu::SurfaceFrame*)
{
    detail::DiagnosticScope diagnosticScope("metal", "WindowSurface.endFrame", _diagnosticIdentity.value);
}

#endif

}
}
}
