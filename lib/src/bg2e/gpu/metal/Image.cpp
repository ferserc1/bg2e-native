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

#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

void Image::buildTargetImage(metal::Device* device, const Size2D& size, PixelFormat format)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _isDepth = false;

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(toMetalPixelFormat(format));
    desc->setWidth(size.width);
    desc->setHeight(size.height);
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

    _texture = device->handle()->newTexture(desc);
    desc->release();

    if (!_texture)
    {
        throw std::runtime_error("metal::Image::buildTargetImage: newTexture failed");
    }
}

void Image::buildDepthImage(metal::Device* device, const Size2D& size, PixelFormat format)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _isDepth = true;

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(toMetalPixelFormat(format));
    desc->setWidth(size.width);
    desc->setHeight(size.height);
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

    _texture = device->handle()->newTexture(desc);
    desc->release();

    if (!_texture)
    {
        throw std::runtime_error("metal::Image::buildDepthImage: newTexture failed");
    }
}

void Image::resize(const Size2D& size)
{
    metal::Device* storedDevice = _device;
    PixelFormat storedFormat = _pixelFormat;
    bool wasDepth = _isDepth;

    cleanup();

    _size = size;

    if (wasDepth)
    {
        buildDepthImage(storedDevice, size, storedFormat);
    }
    else
    {
        buildTargetImage(storedDevice, size, storedFormat);
    }
}

void Image::cleanup()
{
    if (_texture)
    {
        _texture->release();
        _texture = nullptr;
    }
}

bool Image::isValid() const
{
    return _texture != nullptr;
}

#else

void Image::buildTargetImage(metal::Device*, const Size2D&, PixelFormat)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::buildDepthImage(metal::Device*, const Size2D&, PixelFormat)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::resize(const Size2D&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::cleanup() {}

bool Image::isValid() const { return false; }

#endif

}
}
}
