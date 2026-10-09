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
    _size = Size2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

void WindowSurface::cleanup()
{
    releaseRenderTarget();
    if (_metalView)
    {
        SDL_Metal_DestroyView(_metalView);
        _metalView = nullptr;
    }
    _layer  = nullptr;
    _size   = Size2D{};
}

uint32_t WindowSurface::width()  const { return _size.width;  }
uint32_t WindowSurface::height() const { return _size.height; }

bool WindowSurface::isValid() const
{
    return _layer != nullptr;
}

void WindowSurface::createRenderTarget(gpu::Device* device, gpu::PhysicalDevice* /*physicalDevice*/)
{
    _device = device;
    _metalDevice = dynamic_cast<metal::Device*>(device);

    _layer->setDevice(_metalDevice->handle());
    _layer->setPixelFormat(toMetalPixelFormat(_colorFormat));
    _layer->setDrawableSize(CGSize{ double(_size.width), double(_size.height) });
    _layer->setMaximumDrawableCount(3);
    _layer->setFramebufferOnly(false);
    _imageCount = 3;

    if (_size.width && _size.height) createDepthTarget(_size, _depthFormat);
    ++_generation;
}

void WindowSurface::resize(const Size2D& size)
{
    if (_device) _device->waitIdle();
    for (auto& frame : _frames) { if (frame) frame->waitForSubmissions(); frame.reset(); }
    _currentFrame.reset();
    _size = size;
    _layer->setDrawableSize(CGSize{ double(size.width), double(size.height) });
    if (size.width && size.height) {
        if (_depthImage) resizeDepthTarget(size);
        else createDepthTarget(size, _depthFormat);
    }
    ++_generation;
}

void WindowSurface::releaseRenderTarget()
{
    if (_device && _device->isValid()) _device->waitIdle();
    for (auto& frame : _frames) { if (frame) frame->waitForSubmissions(); frame.reset(); }
    _currentFrame.reset();
    releaseDepthTarget();
}

uint32_t WindowSurface::imageCount() const { return _imageCount; }
uint32_t WindowSurface::inFlightFrames() const { return 2; }
uint32_t WindowSurface::currentFrameIndex() const { return _currentFrameIndex; }
gpu::Image* WindowSurface::colorImage(uint32_t /*index*/) const { return nullptr; }
gpu::Image* WindowSurface::depthImage() const { return _depthImage.get(); }

std::shared_ptr<gpu::SurfaceFrame> WindowSurface::beginFrame()
{
    if (_currentFrame) throw std::logic_error("Metal surface already has an acquired frame");
    if (!_size.width || !_size.height || !_layer) return nullptr;
    auto& slot = _frames[_currentFrameIndex];
    if (slot) slot->waitForSubmissions();
    slot.reset();
    auto* drawable = _layer->nextDrawable();
    if (!drawable) return nullptr;
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
    auto* command = dynamic_cast<metal::CommandBuffer*>(cmd);
    if (!command || !_currentFrame || !_currentFrame->isValid())
        throw std::logic_error("Metal presentation requires a valid acquired frame and command buffer");
    command->associateFrame(_currentFrame);
    command->handle()->presentDrawable(_currentFrame->drawable());
}

void WindowSurface::endFrame(gpu::SurfaceFrame* frame)
{
    if (!_currentFrame || frame != _currentFrame.get() || !frame->hasSubmissions())
        throw std::logic_error("Metal endFrame requires a submitted acquired frame");
    _currentFrame.reset();
    _currentFrameIndex = (_currentFrameIndex + 1) % 2;
    ++_frameCounter;
}

#else

void WindowSurface::create(gpu::Instance*)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void WindowSurface::cleanup() {}
uint32_t WindowSurface::width()  const { return 0; }
uint32_t WindowSurface::height() const { return 0; }
bool WindowSurface::isValid()    const { return false; }

void WindowSurface::createRenderTarget(gpu::Device*, gpu::PhysicalDevice*)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void WindowSurface::resize(const Size2D&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void WindowSurface::releaseRenderTarget() {}

uint32_t WindowSurface::imageCount() const { return 0; }
uint32_t WindowSurface::inFlightFrames() const { return 0; }
uint32_t WindowSurface::currentFrameIndex() const { return 0; }
gpu::Image* WindowSurface::colorImage(uint32_t) const { return nullptr; }
gpu::Image* WindowSurface::depthImage() const { return nullptr; }

std::shared_ptr<gpu::SurfaceFrame> WindowSurface::beginFrame() { return nullptr; }
void WindowSurface::present(gpu::CommandBuffer*) {}
void WindowSurface::endFrame(gpu::SurfaceFrame*) {}

#endif

}
}
}
