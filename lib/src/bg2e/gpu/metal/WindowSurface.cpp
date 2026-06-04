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
    SDL_GetWindowSize(window, &w, &h);
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
    auto* metalDevice = dynamic_cast<metal::Device*>(device);

    _layer->setDevice(metalDevice->handle());
    _layer->setPixelFormat(toMetalPixelFormat(_colorFormat));
    _layer->setDrawableSize(CGSize{ double(_size.width), double(_size.height) });
    _layer->setMaximumDrawableCount(3);
    _imageCount = 3;

    if (_depthFormat != PixelFormat::Undefined)
    {
        _depthImage = std::make_unique<metal::Image>();
        _depthImage->buildDepthImage(metalDevice, _size, _depthFormat);
    }
}

void WindowSurface::resize(const Size2D& size)
{
    _size = size;
    _layer->setDrawableSize(CGSize{ double(_size.width), double(_size.height) });
    if (_depthImage)
    {
        _depthImage->resize(size);
    }
}

void WindowSurface::releaseRenderTarget()
{
    _depthImage.reset();
}

uint32_t WindowSurface::imageCount() const { return _imageCount; }
gpu::Image* WindowSurface::colorImage(uint32_t /*index*/) const { return nullptr; }
gpu::Image* WindowSurface::depthImage() const { return _depthImage.get(); }

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
gpu::Image* WindowSurface::colorImage(uint32_t) const { return nullptr; }
gpu::Image* WindowSurface::depthImage() const { return nullptr; }

#endif

}
}
}
