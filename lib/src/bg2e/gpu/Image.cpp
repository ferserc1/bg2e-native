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

#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/base/Image.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace bg2e::gpu {

void Image::validateDescription(const ImageDescription& description)
{
    const auto maximum = mipLevelsForMinSize(std::max(description.size.width, description.size.height), 1);
    const auto levels = effectiveMipLevels(description);
    if (description.size.isZero() || levels == 0 || levels > maximum ||
        pixelFormatBytes(description.format) == 0 || description.usage == ImageUsage::None ||
        hasFlag(description.usage, ImageUsage::Present) ||
        (isDepthFormat(description.format) && hasFlag(description.usage, ImageUsage::ColorAttachment)) ||
        (!isDepthFormat(description.format) && hasFlag(description.usage, ImageUsage::DepthStencil)) ||
        (description.type != ImageType::Image2D && description.type != ImageType::Cubemap) ||
        (description.type == ImageType::Cubemap && (description.size.width != description.size.height || isDepthFormat(description.format))))
    {
        throw std::invalid_argument("Invalid image extent, type or mip count");
    }
}

Size2D Image::mipExtent(uint32_t mipLevel) const
{
    if (mipLevel >= _mipLevels || mipLevel >= 32)
    {
        throw std::out_of_range("Image mip level is out of range");
    }
    return {std::max(1u, width() >> mipLevel), std::max(1u, height() >> mipLevel)};
}

void Image::validateRange(const ImageSubresourceRange& range) const
{
    if (!isValid() || range.mipCount == 0 || range.layerCount == 0 ||
        range.baseMipLevel >= _mipLevels || range.mipCount > _mipLevels - range.baseMipLevel ||
        range.baseArrayLayer >= arrayLayers() || range.layerCount > arrayLayers() - range.baseArrayLayer)
    {
        throw std::out_of_range("Image subresource range is invalid");
    }
}

ImageView Image::view(const ImageSubresourceRange& range)
{
    validateRange(range);
    if (range.layerCount != 1 && !(isCubemap() && range.baseArrayLayer == 0 && range.layerCount == 6))
    {
        throw std::invalid_argument("A view must select one 2D layer or all cube faces");
    }
    return {this, range};
}

ImageView Image::fullView()
{
    return view(fullRange());
}

ImageLayout Image::currentLayout() const
{
    return currentLayout(fullRange());
}

ImageLayout Image::currentLayout(const ImageSubresourceRange& range) const
{
    validateRange(range);
    if (_subresourceLayouts.empty())
    {
        return _currentLayout;
    }
    const auto first = _subresourceLayouts[range.baseArrayLayer * _mipLevels + range.baseMipLevel];
    for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount; ++layer)
    {
        for (uint32_t mip = range.baseMipLevel; mip < range.baseMipLevel + range.mipCount; ++mip)
        {
            if (_subresourceLayouts[layer * _mipLevels + mip] != first)
            {
                throw std::logic_error("Image range has mixed layouts; query individual subresources");
            }
        }
    }
    return first;
}

void Image::resetLayoutTracking()
{
    ++_bindingGeneration;
    _subresourceLayouts.clear();
    _currentLayout = ImageLayout::Undefined;
}

void Image::setLayout(const ImageSubresourceRange& range, ImageLayout layout)
{
    validateRange(range);
    if (_subresourceLayouts.empty())
    {
        _subresourceLayouts.assign(_mipLevels * arrayLayers(), _currentLayout);
    }
    for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount; ++layer)
    {
        for (uint32_t mip = range.baseMipLevel; mip < range.baseMipLevel + range.mipCount; ++mip)
        {
            _subresourceLayouts[layer * _mipLevels + mip] = layout;
        }
    }
}

void Image::validateLayout(ImageLayout layout) const
{
    ImageUsage required = ImageUsage::None;
    switch (layout)
    {
    case ImageLayout::ColorAttachment:
        required = ImageUsage::ColorAttachment;
        break;
    case ImageLayout::DepthAttachment:
        required = ImageUsage::DepthStencil;
        break;
    case ImageLayout::ShaderReadOnly:
        required = ImageUsage::Sampled;
        break;
    case ImageLayout::TransferSrc:
        required = ImageUsage::TransferSrc;
        break;
    case ImageLayout::TransferDst:
        required = ImageUsage::TransferDst;
        break;
    case ImageLayout::General:
        required = ImageUsage::Storage;
        break;
    case ImageLayout::Present:
        required = ImageUsage::Present;
        break;
    default:
        throw std::invalid_argument("Undefined/invalid image layout cannot be a transition destination");
    }
    if (!hasFlag(usage(), required))
    {
        throw std::invalid_argument("Image was not created for the requested layout/usage");
    }
}

size_t Image::validateUpload(const ImageUploadDescription& description) const
{
    validateRange({description.mipLevel, 1, description.arrayLayer, 1});
    if (!description.data || isDepthFormat(_pixelFormat) || !hasFlag(usage(), ImageUsage::TransferDst))
    {
        throw std::invalid_argument("Image upload requires color data and TransferDst usage");
    }
    const auto extent = mipExtent(description.mipLevel);
    const size_t tight = size_t(extent.width) * pixelFormatBytes(_pixelFormat);
    const size_t stride = description.bytesPerRow ? description.bytesPerRow : tight;
    if (!tight || stride < tight || stride % pixelFormatBytes(_pixelFormat) ||
        stride > std::numeric_limits<size_t>::max() / extent.height ||
        description.byteSize < stride * (extent.height - 1) + tight)
    {
        throw std::invalid_argument("Image upload byte size/row pitch is invalid");
    }
    return stride;
}

void Image::upload(const ImageUploadDescription&, ImageLayout)
{
    throw std::runtime_error("Image upload is not implemented by this backend");
}

void Image::readPixels(std::vector<uint8_t>&, const ImageSubresourceRange&, ImageLayout)
{
    throw std::runtime_error("Image readback is not implemented by this backend");
}

void Image::generateMipmaps(ImageLayout finalLayout)
{
    auto* owner = device();
    if (!owner)
    {
        throw std::logic_error("Mipmap generation requires an image device");
    }
    CommandBuffer::validateMipmapGeneration(*this, *owner, finalLayout);
    owner->waitIdle();
    owner->immediateSubmit([&](CommandBuffer* command)
    {
        command->generateMipmaps(this, finalLayout);
    });
}

namespace {
uint16_t floatToHalf(float value)
{
    const uint32_t bits = std::bit_cast<uint32_t>(value);
    const uint32_t sign = (bits >> 16) & 0x8000;
    const uint32_t mantissa = bits & 0x7fffff;
    const int exponent = int((bits >> 23) & 255) - 127 + 15;
    if (((bits >> 23) & 255) == 255)
    {
        return static_cast<uint16_t>(sign | (mantissa ? 0x7e00 : 0x7c00));
    }
    if (exponent <= 0)
    {
        if (exponent < -10)
        {
            return static_cast<uint16_t>(sign);
        }
        const uint32_t shifted = (mantissa | 0x800000) >> (1 - exponent);
        return static_cast<uint16_t>(sign | ((shifted + 0xfff + ((shifted >> 13) & 1)) >> 13));
    }
    const uint32_t rounded = mantissa + 0xfff + ((mantissa >> 13) & 1);
    const int roundedExponent = exponent + ((rounded & 0x800000) ? 1 : 0);
    if (roundedExponent >= 31)
    {
        return static_cast<uint16_t>(sign | 0x7c00);
    }
    return static_cast<uint16_t>(sign | (uint32_t(roundedExponent) << 10) | ((rounded >> 13) & 0x3ff));
}
}

void Image::uploadImage(const base::Image* image)
{
    if (!image || (!image->data() && !image->dataf()) || image->width() != width() || image->height() != height() ||
        image->channels() == 0 || image->channels() > 4 || isCubemap() || isDepthFormat(_pixelFormat) ||
        isSignedIntegerFormat(_pixelFormat) || isUnsignedIntegerFormat(_pixelFormat))
    {
        throw std::invalid_argument("CPU image upload requires matching 2D color dimensions and 1-4 source channels");
    }
    uint32_t channels = 4;
    uint32_t componentBytes = 1;
    bool floatingPoint = false;
    switch (_pixelFormat)
    {
    case PixelFormat::R8_UNORM:
        channels = 1;
        break;
    case PixelFormat::R8G8_UNORM:
        channels = 2;
        break;
    case PixelFormat::R16_SFLOAT:
        channels = 1;
        componentBytes = 2;
        floatingPoint = true;
        break;
    case PixelFormat::R16G16_SFLOAT:
        channels = 2;
        componentBytes = 2;
        floatingPoint = true;
        break;
    case PixelFormat::R16G16B16A16_SFLOAT:
        componentBytes = 2;
        floatingPoint = true;
        break;
    case PixelFormat::R32_SFLOAT:
        channels = 1;
        componentBytes = 4;
        floatingPoint = true;
        break;
    case PixelFormat::R32G32_SFLOAT:
        channels = 2;
        componentBytes = 4;
        floatingPoint = true;
        break;
    case PixelFormat::R32G32B32A32_SFLOAT:
        componentBytes = 4;
        floatingPoint = true;
        break;
    default:
        break;
    }
    // stb file decoding already produces tightly packed RGBA. Preserve those
    // bytes/floats directly when the GPU storage has the identical layout.
    if (image->channels() == 4 && _pixelFormat == PixelFormat::R8G8B8A8_UNORM && image->data())
    {
        upload({image->data(), size_t(width()) * height() * 4});
        return;
    }
    if (image->channels() == 4 && _pixelFormat == PixelFormat::R32G32B32A32_SFLOAT && image->dataf())
    {
        upload({image->dataf(), size_t(width()) * height() * 4 * sizeof(float)});
        return;
    }
    std::vector<uint8_t> packed(size_t(width()) * height() * channels * componentBytes);
    const bool bgra = _pixelFormat == PixelFormat::B8G8R8A8_UNORM || _pixelFormat == PixelFormat::B8G8R8A8_SRGB;
    for (size_t pixel = 0; pixel < size_t(width()) * height(); ++pixel)
    {
        float rgba[4] = {0, 0, 0, 1};
        for (uint32_t channel = 0; channel < image->channels(); ++channel)
        {
            const size_t index = pixel * image->channels() + channel;
            rgba[channel] = image->dataf() ? image->dataf()[index] : image->data()[index] / 255.0f;
        }
        if (image->channels() == 1 || image->channels() == 2)
        {
            rgba[3] = image->channels() == 2 ? rgba[1] : 1;
            rgba[1] = rgba[2] = rgba[0];
        }
        for (uint32_t channel = 0; channel < channels; ++channel)
        {
            const float value = rgba[bgra && channel < 3 ? 2 - channel : channel];
            auto* target = packed.data() + (pixel * channels + channel) * componentBytes;
            if (floatingPoint && componentBytes == 2)
            {
                const auto half = floatToHalf(value);
                std::memcpy(target, &half, sizeof(half));
            }
            else if (floatingPoint)
            {
                std::memcpy(target, &value, sizeof(value));
            }
            else
            {
                *target = std::isfinite(value) ? static_cast<uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255)) : 0;
            }
        }
    }
    upload({packed.data(), packed.size()});
}

}
