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

#include <bg2e/utils/BlueNoise.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/Sampler.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
#include <utility>

// One precomputed rank source during production/draw coexistence. This private
// header contains bytes/metadata only; no render API or native Vulkan dependency.
#include "../render/blue_noise_data.h"

namespace bg2e::utils {
namespace {
uint32_t hash(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

float fraction(float value)
{
    return value - std::floor(value);
}

uint32_t wrap(int64_t coordinate, uint32_t extent)
{
    const auto value = coordinate % int64_t(extent);
    return static_cast<uint32_t>(value < 0 ? value + extent : value);
}
}

BlueNoiseData BlueNoiseData::embedded()
{
    static_assert(sizeof(bg2e_blue_noise_data) == size_t(BG2E_BLUE_NOISE_WIDTH) *
        BG2E_BLUE_NOISE_HEIGHT * BG2E_BLUE_NOISE_CHANNELS * BG2E_BLUE_NOISE_LAYERS);
    return {{BG2E_BLUE_NOISE_WIDTH, BG2E_BLUE_NOISE_HEIGHT}, BG2E_BLUE_NOISE_CHANNELS,
        BG2E_BLUE_NOISE_LAYERS, std::span<const uint8_t>(bg2e_blue_noise_data)};
}

void BlueNoiseData::validate() const
{
    if (!tileSize.width || !tileSize.height || !layers || channels < 1 || channels > 4 || !bytes.data())
    {
        throw std::invalid_argument("BlueNoise requires nonempty tile/layer data and 1..4 channels");
    }
    size_t expected = tileSize.width;
    for (uint32_t factor : {tileSize.height, layers, channels})
    {
        if (expected > std::numeric_limits<size_t>::max() / factor)
        {
            throw std::invalid_argument("BlueNoise byte size overflows addressable memory");
        }
        expected *= factor;
    }
    if (expected != bytes.size())
    {
        throw std::invalid_argument("BlueNoise byte count differs from tile/channel/layer metadata");
    }
}

std::array<uint8_t, 4> BlueNoiseData::texel(int64_t x, int64_t y, uint32_t layer) const
{
    validate();
    if (layer >= layers)
    {
        throw std::out_of_range("BlueNoise logical layer is out of range");
    }
    const auto index = ((size_t(layer) * tileSize.height + wrap(y, tileSize.height)) * tileSize.width +
        wrap(x, tileSize.width)) * channels;
    std::array<uint8_t, 4> result{0, 0, 0, 255};
    for (uint32_t channel = 0; channel < channels; ++channel)
    {
        result[channel] = bytes[index + channel];
    }
    return result;
}

// Deliberate uint32 wrapping matches paired native shader arithmetic.
glm::vec2 BlueNoiseData::sample2D(int32_t x, int32_t y, uint32_t renderedFrame, uint32_t sample) const
{
    validate();
    if (channels < 2)
    {
        throw std::invalid_argument("BlueNoise 2D samples require at least two source channels");
    }
    const uint32_t sequence = (renderedFrame * 233u + sample * 61u) & 65535u;
    const uint32_t layer = (renderedFrame + sample * 7u) % layers;
    const auto rank = texel(x, y, layer);
    return {fraction((float(rank[0]) + 0.5f) / 256.0f + fraction(0.7548776662f * float(sequence))),
            fraction((float(rank[1]) + 0.5f) / 256.0f + fraction(0.56984029f * float(sequence)))};
}

glm::vec2 sampleNoise2D(const BlueNoiseData* data, int32_t x, int32_t y,
                       uint32_t renderedFrame, uint32_t sample)
{
    if (data)
    {
        return data->sample2D(x, y, renderedFrame, sample);
    }
    const auto seed = hash(uint32_t(x)) ^ hash(uint32_t(y) + 0x9e3779b9u) ^
        hash(renderedFrame) ^ hash(sample + 0x85ebca6bu);
    return {float(hash(seed) >> 9) / 8388608.0f, float(hash(seed ^ 0x68bc21ebu) >> 9) / 8388608.0f};
}

BlueNoise::BlueNoise(gpu::Device& device)
    : _device(&device)
{
}

void BlueNoise::build(const BlueNoiseData& data)
{
    data.validate();
    if (!_device->isValid())
    {
        throw std::invalid_argument("BlueNoise requires an initialized device");
    }
    const auto atlasHeight = uint64_t(data.tileSize.height) * data.layers;
    if (atlasHeight > uint64_t(std::numeric_limits<int32_t>::max()) ||
        data.tileSize.width > uint32_t(std::numeric_limits<int32_t>::max()) ||
        uint64_t(data.tileSize.width) * atlasHeight > std::numeric_limits<size_t>::max() / 4)
    {
        throw std::invalid_argument("BlueNoise atlas dimensions overflow");
    }
    const auto usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst | gpu::ImageUsage::TransferSrc;
    if (!_device->formatCapabilities(gpu::PixelFormat::R8G8B8A8_UNORM).supports(usage))
    {
        throw std::runtime_error("BlueNoise requires sampled/transfer RGBA8 UNORM support");
    }
    std::vector<uint8_t> pixels(size_t(data.tileSize.width) * size_t(atlasHeight) * 4);
    for (size_t texel = 0; texel < pixels.size() / 4; ++texel)
    {
        pixels[texel * 4 + 3] = 255;
        for (uint32_t channel = 0; channel < data.channels; ++channel)
        {
            pixels[texel * 4 + channel] = data.bytes[texel * data.channels + channel];
        }
    }
    // Rare replacement only. New native resources are published together after
    // successful synchronous upload; failures preserve the previous snapshot.
    _device->waitIdle();
    gpu::ImageDescription description;
    description.size = {data.tileSize.width, static_cast<uint32_t>(atlasHeight)};
    description.format = gpu::PixelFormat::R8G8B8A8_UNORM;
    description.usage = usage;
    description.debugName = "BlueNoise logical-layer atlas";
    auto image = _device->createImage(description);
    gpu::SamplerDescription samplerDescription;
    samplerDescription.minFilter = gpu::Filter::Nearest;
    samplerDescription.magFilter = gpu::Filter::Nearest;
    samplerDescription.mipFilter = gpu::MipmapFilter::Nearest;
    samplerDescription.addressModeU = gpu::AddressMode::Repeat;
    samplerDescription.addressModeV = gpu::AddressMode::Repeat;
    samplerDescription.minLod = 0;
    samplerDescription.maxLod = 0;
    samplerDescription.debugName = "BlueNoise nearest repeat";
    auto sampler = _device->createSampler(samplerDescription);
    if (!image || !sampler || !image->isValid() || !sampler->isValid() ||
        image->device() != _device || sampler->device() != _device)
    {
        throw std::runtime_error("BlueNoise native resource creation returned invalid resources");
    }
    image->upload({pixels.data(), pixels.size(), size_t(data.tileSize.width) * 4, 0, 0});
    _image = std::move(image);
    _sampler = std::move(sampler);
    _metadata = {data.tileSize.width, data.tileSize.height, data.channels, data.layers};
}

void BlueNoise::cleanup()
{
    if (_image || _sampler)
    {
        _device->waitIdle();
    }
    // Drop this object's references only; exported shared owners remain usable.
    _image.reset();
    _sampler.reset();
    _metadata = glm::uvec4(0);
}

bool BlueNoise::isReady() const
{
    return _image && _sampler && _image->isValid() && _sampler->isValid();
}

gpu::Device& BlueNoise::device() const
{
    return *_device;
}

gpu::Size2D BlueNoise::tileSize() const
{
    return {_metadata.x, _metadata.y};
}

uint32_t BlueNoise::channelCount() const
{
    return _metadata.z;
}

uint32_t BlueNoise::layerCount() const
{
    return _metadata.w;
}

glm::uvec4 BlueNoise::shaderMetadata() const
{
    return _metadata;
}

const std::shared_ptr<gpu::Image>& BlueNoise::image() const
{
    return _image;
}

const std::shared_ptr<gpu::Sampler>& BlueNoise::sampler() const
{
    return _sampler;
}
}
