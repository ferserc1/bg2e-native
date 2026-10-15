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
#include <algorithm>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

ImageUsage Image::usage() const
{
    return _usage;
}

#if BG2E_IS_MAC

void Image::buildImage(metal::Device* device, const ImageDescription& description)
{
    gpu::Image::validateDescription(description);
    if (!device || !device->isValid() || !device->formatCapabilities(description.format).supports(description.usage))
    {
        throw std::invalid_argument("Metal image device/format/usage is unsupported");
    }
    if (isValid() && _device)
    {
        _device->waitIdle();
    }
    cleanup();
    _device = device;
    DeviceResource::_device = device;
    _size = description.size;
    _pixelFormat = description.format;
    _imageType = description.type;
    _mipLevels = effectiveMipLevels(description);
    _usage = description.usage;
    _isDepth = isDepthFormat(_pixelFormat);
    _ownsTexture = true;
    _debugName = description.debugName;
    resetLayoutTracking();
    auto* descriptor = MTL::TextureDescriptor::alloc()->init();
    descriptor->setTextureType(isCubemap() ? MTL::TextureTypeCube : MTL::TextureType2D);
    descriptor->setPixelFormat(toMetalPixelFormat(_pixelFormat));
    descriptor->setWidth(width());
    descriptor->setHeight(height());
    descriptor->setMipmapLevelCount(_mipLevels);
    descriptor->setStorageMode(MTL::StorageModePrivate);
    MTL::TextureUsage nativeUsage = MTL::TextureUsagePixelFormatView;
    if (hasFlag(_usage, ImageUsage::Sampled))
    {
        nativeUsage |= MTL::TextureUsageShaderRead;
    }
    if (hasFlag(_usage, ImageUsage::Storage))
    {
        nativeUsage |= MTL::TextureUsageShaderWrite;
    }
    if (hasFlag(_usage, ImageUsage::ColorAttachment) || hasFlag(_usage, ImageUsage::DepthStencil))
    {
        nativeUsage |= MTL::TextureUsageRenderTarget;
    }
    descriptor->setUsage(nativeUsage);
    descriptor->setHazardTrackingMode(MTL::HazardTrackingModeTracked);
    _texture = device->handle()->newTexture(descriptor);
    descriptor->release();
    if (!_texture)
    {
        throw std::runtime_error("Metal image allocation failed for the requested format/usage/mips");
    }
    if (base::Log::isDebug() && !_debugName.empty())
    {
        _texture->setLabel(NS::String::string(_debugName.c_str(), NS::UTF8StringEncoding));
    }
}

void Image::buildTargetImage(metal::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    buildImage(device, {.size = size, .format = format,
        .usage = ImageUsage::ColorAttachment | ImageUsage::Sampled | ImageUsage::TransferSrc | ImageUsage::TransferDst,
        .debugName = debugName});
}
void Image::buildDepthImage(metal::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    buildImage(device, {.size = size, .format = format,
        .usage = ImageUsage::DepthStencil | ImageUsage::Sampled, .debugName = debugName});
}
void Image::buildSampledImage(metal::Device* device, const Size2D& size, PixelFormat format,
                              const std::string& debugName, ImageUsage usage)
{
    buildImage(device, {.size = size, .format = format, .usage = usage, .debugName = debugName});
}
void Image::buildStorageImage(metal::Device* device, const Size2D& size, PixelFormat format, const std::string& debugName)
{
    buildImage(device, {.size = size, .format = format,
        .usage = ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferSrc | ImageUsage::TransferDst,
        .debugName = debugName});
}
void Image::buildCubemapImage(metal::Device* device, const Size2D& size, PixelFormat format,
                              MTL::TextureUsage usage, uint32_t levels, const std::string& debugName)
{
    ImageUsage generic = ImageUsage::TransferSrc | ImageUsage::TransferDst;
    if (usage & MTL::TextureUsageShaderRead)
    {
        generic |= ImageUsage::Sampled;
    }
    if (usage & MTL::TextureUsageShaderWrite)
    {
        generic |= ImageUsage::Storage;
    }
    if (usage & MTL::TextureUsageRenderTarget)
    {
        generic |= ImageUsage::ColorAttachment;
    }
    buildImage(device, {.size = size, .format = format, .usage = generic,
        .type = ImageType::Cubemap, .mipLevels = levels, .debugName = debugName});
}

TextureHandle Image::subresourceView(const ImageSubresourceRange& range)
{
    view(range); // Validate the supported view shape as well as its bounds.
    if (range == fullRange())
    {
        return _texture;
    }
    const std::array<uint32_t, 4> key{range.baseMipLevel, range.mipCount, range.baseArrayLayer, range.layerCount};
    const auto found = _subresourceViews.find(key);
    if (found != _subresourceViews.end())
    {
        return found->second;
    }
    auto* result = _texture->newTextureView(toMetalPixelFormat(_pixelFormat),
        range.layerCount == 6 ? MTL::TextureTypeCube : MTL::TextureType2D,
        NS::Range{range.baseMipLevel, range.mipCount}, NS::Range{range.baseArrayLayer, range.layerCount});
    if (!result)
    {
        throw std::runtime_error("Metal subresource texture view creation failed");
    }
    _subresourceViews.emplace(key, result);
    return result;
}

void Image::initFromDrawableTexture(metal::Device* device, TextureHandle texture, PixelFormat format, const Size2D& size)
{
    cleanup();
    _device = device;
    DeviceResource::_device = device;
    _texture = texture;
    _size = size;
    _pixelFormat = format;
    _imageType = ImageType::Image2D;
    _mipLevels = 1;
    _ownsTexture = false;
    _isDepth = false;
    _usage = ImageUsage::ColorAttachment | ImageUsage::Present;
    if (_texture && (_texture->usage() & MTL::TextureUsageShaderRead))
    {
        _usage |= ImageUsage::Sampled;
    }
    if (_texture && (_texture->usage() & MTL::TextureUsageShaderWrite))
    {
        _usage |= ImageUsage::Storage;
    }
    if (_texture && !_texture->isFramebufferOnly())
    {
        _usage |= ImageUsage::TransferSrc | ImageUsage::TransferDst;
    }
    resetLayoutTracking();
}

bool Image::isValidCubemapFaceMip(uint32_t face, uint32_t mipLevel) const
{
    return isCubemap() && isValid() && face < 6 && mipLevel < _mipLevels;
}
Image::CubemapFaceMipTarget Image::cubemapFaceMipTarget(uint32_t face, uint32_t mipLevel) const
{
    if (!isValidCubemapFaceMip(face, mipLevel))
    {
        return {nullptr, 0, 0};
    }
    return {_texture, face, mipLevel};
}

void Image::resize(const Size2D& size)
{
    if (!_ownsTexture)
    {
        throw std::logic_error("Cannot resize a wrapped image");
    }
    ImageDescription description{.size = size, .format = _pixelFormat, .usage = _usage, .type = _imageType,
        .mipLevels = std::min(_mipLevels, mipLevelsForMinSize(std::max(size.width, size.height), 1)), .debugName = _debugName};
    buildImage(_device, description);
}

void Image::cleanup()
{
    for (const auto& entry : _subresourceViews)
    {
        entry.second->release();
    }
    _subresourceViews.clear();
    if (_texture && _ownsTexture)
    {
        _texture->release();
    }
    _texture = nullptr;
    _usage = ImageUsage::None;
    resetLayoutTracking();
}

bool Image::isValid() const
{
    return _texture != nullptr;
}

void Image::upload(const ImageUploadDescription& description, ImageLayout finalLayout)
{
    const size_t sourceStride = validateUpload(description);
    validateLayout(finalLayout);
    _device->waitIdle();
    const auto extent = mipExtent(description.mipLevel);
    const size_t tight = size_t(extent.width) * pixelFormatBytes(_pixelFormat);
    const size_t stride = (tight + 255) & ~size_t(255);
    const size_t bytes = stride * extent.height;
    auto* staging = _device->handle()->newBuffer(bytes, MTL::ResourceStorageModeShared);
    if (!staging)
    {
        throw std::runtime_error("Metal upload staging buffer allocation failed");
    }
    for (uint32_t row = 0; row < extent.height; ++row)
    {
        std::memcpy(static_cast<uint8_t*>(staging->contents()) + row * stride,
            static_cast<const uint8_t*>(description.data) + row * sourceStride, tight);
    }
    const ImageSubresourceRange range{description.mipLevel, 1, description.arrayLayer, 1};
    try
    {
        _device->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(this, ImageLayout::TransferDst, range);
            auto* encoder = static_cast<metal::CommandBuffer*>(command)->handle()->blitCommandEncoder();
            if (!encoder)
            {
                throw std::runtime_error("Metal upload blit encoder creation failed");
            }
            encoder->copyFromBuffer(staging, 0, stride, bytes, MTL::Size{extent.width, extent.height, 1},
                _texture, range.baseArrayLayer, range.baseMipLevel, MTL::Origin{0, 0, 0});
            encoder->endEncoding(); // Autoreleased; no owned release.
            command->transition(this, finalLayout, range);
        });
    }
    catch (...)
    {
        staging->release();
        throw;
    }
    staging->release();
}

void Image::readPixels(std::vector<uint8_t>& data, const ImageSubresourceRange& range, ImageLayout finalLayout)
{
    validateRange(range);
    validateLayout(finalLayout);
    if (range.mipCount != 1 || range.layerCount != 1 || _isDepth || !hasFlag(_usage, ImageUsage::TransferSrc) ||
        currentLayout(range) == ImageLayout::Undefined)
    {
        throw std::invalid_argument("Readback requires one initialized color subresource with TransferSrc usage");
    }
    _device->waitIdle();
    const auto extent = mipExtent(range.baseMipLevel);
    const size_t tight = size_t(extent.width) * pixelFormatBytes(_pixelFormat);
    const size_t stride = (tight + 255) & ~size_t(255);
    const size_t bytes = stride * extent.height;
    auto* staging = _device->handle()->newBuffer(bytes, MTL::ResourceStorageModeShared);
    if (!staging)
    {
        throw std::runtime_error("Metal readback staging buffer allocation failed");
    }
    try
    {
        _device->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(this, ImageLayout::TransferSrc, range);
            auto* encoder = static_cast<metal::CommandBuffer*>(command)->handle()->blitCommandEncoder();
            if (!encoder)
            {
                throw std::runtime_error("Metal readback blit encoder creation failed");
            }
            encoder->copyFromTexture(_texture, range.baseArrayLayer, range.baseMipLevel, MTL::Origin{0, 0, 0},
                MTL::Size{extent.width, extent.height, 1}, staging, 0, stride, bytes);
            encoder->endEncoding();
            command->transition(this, finalLayout, range);
        });
        data.resize(tight * extent.height);
        for (uint32_t row = 0; row < extent.height; ++row)
        {
            std::memcpy(data.data() + row * tight, static_cast<const uint8_t*>(staging->contents()) + row * stride, tight);
        }
    }
    catch (...)
    {
        staging->release();
        throw;
    }
    staging->release();
}

void Image::readPixelsRGBA8(std::vector<uint8_t>& data, ImageLayout finalLayout)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM)
    {
        throw std::invalid_argument("readPixelsRGBA8 requires R8G8B8A8_UNORM");
    }
    readPixels(data, {}, finalLayout);
}
void Image::uploadRGBA8(const void* pixels, const Size2D& size)
{
    if (_pixelFormat != PixelFormat::R8G8B8A8_UNORM || size != _size)
    {
        throw std::invalid_argument("uploadRGBA8 requires matching RGBA8 dimensions");
    }
    upload({pixels, size_t(width()) * height() * 4});
}
void Image::uploadImage(const base::Image* image)
{
    gpu::Image::uploadImage(image);
}

#else

void Image::buildImage(metal::Device*, const ImageDescription&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
void Image::buildTargetImage(metal::Device*, const Size2D&, PixelFormat, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
void Image::buildDepthImage(metal::Device*, const Size2D&, PixelFormat, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
void Image::buildSampledImage(metal::Device*, const Size2D&, PixelFormat, const std::string&, ImageUsage)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
void Image::buildStorageImage(metal::Device*, const Size2D&, PixelFormat, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
void Image::buildCubemapImage(metal::Device*, const Size2D&, PixelFormat, TextureUsageEnum, uint32_t, const std::string&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
TextureHandle Image::subresourceView(const ImageSubresourceRange&)
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
bool Image::isValidCubemapFaceMip(uint32_t, uint32_t) const
{
    return false;
}
Image::CubemapFaceMipTarget Image::cubemapFaceMipTarget(uint32_t, uint32_t) const
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
void Image::cleanup()
{
    resetLayoutTracking();
}
bool Image::isValid() const
{
    return false;
}
void Image::upload(const ImageUploadDescription&, ImageLayout)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
void Image::readPixels(std::vector<uint8_t>&, const ImageSubresourceRange&, ImageLayout)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
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
