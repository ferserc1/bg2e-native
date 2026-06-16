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
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/metal/common.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/base/Image.hpp>

#include <cstring>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

void Image::buildTargetImage(metal::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _isDepth = false;
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(toMetalPixelFormat(format));
    desc->setWidth(size.width);
    desc->setHeight(size.height);
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

    _texture = device->handle()->newTexture(desc);
    _ownsTexture = true;
    desc->release();

    if (!_texture)
    {
        throw std::runtime_error("metal::Image::buildTargetImage: newTexture failed");
    }

    if (base::Log::isDebug() && !debugName.empty())
    {
        _texture->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Image::buildDepthImage(metal::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _isDepth = true;
    _ownsTexture = true;
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(toMetalPixelFormat(format));
    desc->setWidth(size.width);
    desc->setHeight(size.height);
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

    _texture = device->handle()->newTexture(desc);
    _ownsTexture = true;
    desc->release();

    if (!_texture)
    {
        throw std::runtime_error("metal::Image::buildDepthImage: newTexture failed");
    }

    if (base::Log::isDebug() && !debugName.empty())
    {
        _texture->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Image::buildSampledImage(metal::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _isDepth = false;
    _ownsTexture = true;
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(toMetalPixelFormat(format));
    desc->setWidth(size.width);
    desc->setHeight(size.height);
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(MTL::TextureUsageShaderRead);

    _texture = device->handle()->newTexture(desc);
    desc->release();

    if (!_texture)
    {
        throw std::runtime_error("metal::Image::buildSampledImage: newTexture failed");
    }

    if (base::Log::isDebug() && !debugName.empty())
    {
        _texture->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Image::buildStorageImage(metal::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    cleanup();

    _device = device;
    _size = size;
    _pixelFormat = format;
    _isDepth = false;
    _ownsTexture = true;
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureType2D);
    desc->setPixelFormat(toMetalPixelFormat(format));
    desc->setWidth(size.width);
    desc->setHeight(size.height);
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);

    _texture = device->handle()->newTexture(desc);
    _ownsTexture = true;
    desc->release();

    if (!_texture)
    {
        throw std::runtime_error("metal::Image::buildStorageImage: newTexture failed");
    }

    if (base::Log::isDebug() && !debugName.empty())
    {
        _texture->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Image::buildCubemapImage(metal::Device* device, const Size2D& size, PixelFormat format,
                               MTL::TextureUsage usage, uint32_t mipLevels, const std::string& debugName)
{
    cleanup();

    if (size.width != size.height)
    {
        throw std::runtime_error("metal::Image::buildCubemapImage: cubemap requires square dimensions");
    }

    _device = device;
    _size = size;
    _pixelFormat = format;
    _isDepth = false;
    _ownsTexture = true;
    _currentLayout = ImageLayout::Undefined;
    _debugName = debugName;
    _imageType = ImageType::Cubemap;
    _mipLevels = mipLevels;

    auto* desc = MTL::TextureDescriptor::alloc()->init();
    desc->setTextureType(MTL::TextureTypeCube);
    desc->setPixelFormat(toMetalPixelFormat(format));
    desc->setWidth(size.width);
    desc->setHeight(size.height);
    desc->setMipmapLevelCount(mipLevels);
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(usage);

    _texture = device->handle()->newTexture(desc);
    desc->release();

    if (!_texture)
    {
        throw std::runtime_error("metal::Image::buildCubemapImage: newTexture failed");
    }

    if (base::Log::isDebug() && !debugName.empty())
    {
        _texture->setLabel(NS::String::string(debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Image::initFromDrawableTexture(metal::Device* device, TextureHandle texture,
                                     PixelFormat format, const Size2D& size)
{
    cleanup();
    _device        = device;
    _texture       = texture;
    _size          = size;
    _pixelFormat   = format;
    _isDepth       = false;
    _ownsTexture   = false;
    _currentLayout = ImageLayout::Undefined;
}

bool Image::isValidCubemapFaceMip(uint32_t face, uint32_t mipLevel) const
{
    if (_imageType != ImageType::Cubemap) return false;
    if (face >= 6) return false;
    if (mipLevel >= _mipLevels) return false;
    return true;
}

Image::CubemapFaceMipTarget Image::cubemapFaceMipTarget(uint32_t face, uint32_t mipLevel) const
{
    if (!isValidCubemapFaceMip(face, mipLevel)) return { nullptr, 0, 0 };
    return { _texture, face, mipLevel };
}

void Image::resize(const Size2D& size)
{
    metal::Device* storedDevice = _device;
    PixelFormat storedFormat = _pixelFormat;
    bool wasDepth = _isDepth;
    std::string storedDebugName = _debugName;

    cleanup();

    _size = size;

    if (wasDepth)
    {
        buildDepthImage(storedDevice, size, storedFormat, storedDebugName);
    }
    else
    {
        buildTargetImage(storedDevice, size, storedFormat, storedDebugName);
    }
}

void Image::cleanup()
{
    if (_texture && _ownsTexture)
    {
        _texture->release();
        _texture = nullptr;
    }
}

bool Image::isValid() const
{
    return _texture != nullptr;
}

void Image::readPixelsRGBA8(std::vector<uint8_t>& outData, ImageLayout /*currentLayout*/)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM) {
        throw std::runtime_error("metal::Image::readPixelsRGBA8: only R8G8B8A8_UNORM is supported");
    }

    const uint32_t w = _size.width;
    const uint32_t h = _size.height;

    const uint32_t unpaddedBytesPerRow = w * 4;
    const uint32_t alignment = 256;
    const uint32_t paddedBytesPerRow =
        ((unpaddedBytesPerRow + alignment - 1) / alignment) * alignment;
    const size_t bufferSize = size_t(paddedBytesPerRow) * h;

    MTL::Buffer* staging = _device->handle()->newBuffer(
        bufferSize, MTL::ResourceStorageModeShared);
    if (!staging) {
        throw std::runtime_error("metal::Image::readPixelsRGBA8: newBuffer failed");
    }

    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        MTL::CommandBuffer* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd)->handle();
        MTL::BlitCommandEncoder* blit = mtlCmd->blitCommandEncoder();
        blit->copyFromTexture(
            _texture,
            0,
            0,
            MTL::Origin{ 0, 0, 0 },
            MTL::Size{ w, h, 1 },
            staging,
            0,
            paddedBytesPerRow,
            paddedBytesPerRow * h);
        blit->endEncoding();
    });

    outData.resize(size_t(unpaddedBytesPerRow) * h);
    const uint8_t* src = static_cast<const uint8_t*>(staging->contents());
    for (uint32_t y = 0; y < h; ++y) {
        std::memcpy(outData.data() + size_t(y) * unpaddedBytesPerRow,
                    src + size_t(y) * paddedBytesPerRow,
                    unpaddedBytesPerRow);
    }

    staging->release();
}

void Image::uploadRGBA8(const void* pixels, const Size2D& size)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM) {
        throw std::runtime_error("metal::Image::uploadRGBA8: only R8G8B8A8_UNORM is supported");
    }

    if (size.width != _size.width || size.height != _size.height) {
        throw std::runtime_error("metal::Image::uploadRGBA8: size mismatch");
    }

    const uint32_t w = _size.width;
    const uint32_t h = _size.height;
    const uint32_t bytesPerRow = w * 4;
    const size_t bufferSize = size_t(bytesPerRow) * h;

    MTL::Buffer* staging = _device->handle()->newBuffer(bufferSize, MTL::ResourceStorageModeShared);
    if (!staging) {
        throw std::runtime_error("metal::Image::uploadRGBA8: newBuffer failed");
    }

    std::memcpy(staging->contents(), pixels, bufferSize);

    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        MTL::CommandBuffer* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd)->handle();
        MTL::BlitCommandEncoder* blit = mtlCmd->blitCommandEncoder();
        blit->copyFromBuffer(
            staging,
            0,
            bytesPerRow,
            bufferSize,
            MTL::Size{ w, h, 1 },
            _texture,
            0,
            0,
            MTL::Origin{ 0, 0, 0 });
        blit->endEncoding();
    });

    staging->release();
}

static uint16_t floatToHalf(float f)
{
    uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    uint32_t sign = (bits >> 16) & 0x8000;
    int32_t exponent = ((bits >> 23) & 0xFF) - 127;
    uint32_t mantissa = bits & 0x7FFFFF;

    if (exponent > 15) {
        return static_cast<uint16_t>(sign | 0x7C00);
    }
    if (exponent < -14) {
        uint32_t m = (mantissa | 0x8000) >> (-exponent - 14 + 1);
        if (m & 0x1000) m += 0x2000;
        return static_cast<uint16_t>(sign | (m >> 13));
    }
    if (exponent == -127) {
        return static_cast<uint16_t>(sign);
    }
    return static_cast<uint16_t>(sign | ((exponent + 15) << 10) | (mantissa >> 13));
}

void Image::uploadImage(const base::Image* image)
{
    if (!image || (!image->data() && !image->dataf())) {
        throw std::runtime_error("metal::Image::uploadImage: null or empty image");
    }

    const uint32_t w = _size.width;
    const uint32_t h = _size.height;

    if (image->width() != w || image->height() != h) {
        throw std::runtime_error("metal::Image::uploadImage: size mismatch");
    }

    size_t bytesPerPixel = 0;
    const void* srcData = nullptr;
    std::vector<uint16_t> halfData;

    switch (_pixelFormat) {
        case PixelFormat::R8G8B8A8_UNORM:
        case PixelFormat::R8G8B8A8_SRGB:
        case PixelFormat::B8G8R8A8_UNORM:
        case PixelFormat::B8G8R8A8_SRGB:
            bytesPerPixel = 4;
            srcData = image->data();
            break;
        case PixelFormat::R16G16B16A16_SFLOAT:
            bytesPerPixel = 8;
            if (image->dataf()) {
                const float* src = image->dataf();
                size_t pixelCount = size_t(w) * h;
                halfData.resize(pixelCount * 4);
                for (size_t i = 0; i < pixelCount * 4; ++i) {
                    halfData[i] = floatToHalf(src[i]);
                }
                srcData = halfData.data();
            } else {
                const uint8_t* src = image->data();
                size_t pixelCount = size_t(w) * h;
                halfData.resize(pixelCount * 4);
                for (size_t i = 0; i < pixelCount * 4; ++i) {
                    halfData[i] = floatToHalf(src[i] / 255.0f);
                }
                srcData = halfData.data();
            }
            break;
        case PixelFormat::R32G32B32A32_SFLOAT:
            bytesPerPixel = 16;
            srcData = image->dataf();
            break;
        default:
            throw std::runtime_error("metal::Image::uploadImage: unsupported pixel format");
    }

    if (!srcData) {
        throw std::runtime_error("metal::Image::uploadImage: no source data for this format");
    }

    const uint32_t bytesPerRow = w * bytesPerPixel;
    const size_t bufferSize = size_t(bytesPerRow) * h;

    MTL::Buffer* staging = _device->handle()->newBuffer(bufferSize, MTL::ResourceStorageModeShared);
    if (!staging) {
        throw std::runtime_error("metal::Image::uploadImage: newBuffer failed");
    }

    std::memcpy(staging->contents(), srcData, bufferSize);

    _device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
        MTL::CommandBuffer* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd)->handle();
        MTL::BlitCommandEncoder* blit = mtlCmd->blitCommandEncoder();
        blit->copyFromBuffer(
            staging,
            0,
            bytesPerRow,
            bufferSize,
            MTL::Size{ w, h, 1 },
            _texture,
            0,
            0,
            MTL::Origin{ 0, 0, 0 });
        blit->endEncoding();
    });

    staging->release();
}

#else

void Image::buildTargetImage(metal::Device*, const Size2D&, PixelFormat, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::buildDepthImage(metal::Device*, const Size2D&, PixelFormat, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::resize(const Size2D&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::initFromDrawableTexture(metal::Device*, TextureHandle, PixelFormat, const Size2D&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::buildSampledImage(metal::Device*, const Size2D&, PixelFormat, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::buildStorageImage(metal::Device*, const Size2D&, PixelFormat, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::buildCubemapImage(metal::Device*, const Size2D&, PixelFormat, MTL::TextureUsage, uint32_t, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

bool Image::isValidCubemapFaceMip(uint32_t, uint32_t) const
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

Image::CubemapFaceMipTarget Image::cubemapFaceMipTarget(uint32_t, uint32_t) const
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::cleanup() {}

bool Image::isValid() const { return false; }

void Image::readPixelsRGBA8(std::vector<uint8_t>&, ImageLayout)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::uploadRGBA8(const void*, const Size2D&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void Image::uploadImage(const base::Image*)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

#endif

}
}
}
