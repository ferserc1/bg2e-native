/*
 *    business grade graphic engine (bg2 engine)
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

#include <bg2e/draw/pipeline/FrameAccumulator.hpp>
#include <glm/packing.hpp>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bg2e::draw::pipeline {
void FrameAccumulator::reset(gpu::Size2D extent, uint32_t requestedFrames)
{
    if (extent.isZero() || requestedFrames == 0 || requestedFrames > 1048576 ||
        uint64_t(extent.width)*extent.height > std::numeric_limits<size_t>::max()/sizeof(double)/4)
    {
        throw std::invalid_argument("Invalid scene accumulation extent or sample count (1..1048576)");
    }
    std::vector<double> sum(size_t(extent.width)*extent.height*4, 0.0);
    _extent = extent;
    _requested = requestedFrames;
    _completed = 0;
    _sum = std::move(sum);
}

void FrameAccumulator::add(std::span<const float> rgba)
{
    if (!_requested || _completed >= _requested || rgba.size() != _sum.size())
    {
        throw std::logic_error("Scene accumulation sample count or extent mismatch");
    }
    // Validate the entire sample before committing any contribution.
    for (size_t i = 0; i < rgba.size(); ++i)
    {
        if (i%4 != 3 && !std::isfinite(rgba[i]))
        {
            throw std::invalid_argument("Scene accumulation requires finite linear RGB");
        }
    }
    for (size_t i = 0; i < rgba.size(); ++i)
    {
        if (i%4 != 3)
        {
            _sum[i] += double(rgba[i]);
        }
    }
    ++_completed;
}

void FrameAccumulator::add(gpu::Image& sample)
{
    if (sample.size() != _extent || sample.pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT ||
        sample.mipLevels() != 1 || sample.arrayLayers() != 1)
    {
        throw std::invalid_argument("Scene sample must be output-sized single-level RGBA16F");
    }
    std::vector<uint8_t> bytes;
    sample.readPixels(bytes, sample.fullRange());
    if (bytes.size() != _sum.size()*sizeof(uint16_t))
    {
        throw std::runtime_error("Unexpected scene sample readback size");
    }
    std::vector<float> values(_sum.size());
    for (size_t i = 0; i < values.size(); i += 2)
    {
        uint32_t packed;
        std::memcpy(&packed, bytes.data()+i*sizeof(uint16_t), sizeof(packed));
        auto pair = glm::unpackHalf2x16(packed);
        values[i] = pair.x;
        values[i+1] = pair.y;
    }
    add(values);
}

std::vector<float> FrameAccumulator::resolve() const
{
    if (!complete())
    {
        throw std::logic_error("Scene average is unavailable before all requested samples complete");
    }
    std::vector<float> result(_sum.size());
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = i%4 == 3 ? 1.0f : float(_sum[i]/double(_requested));
    }
    return result;
}

void FrameAccumulator::resolveInto(gpu::Image& destination) const
{
    if (destination.size() != _extent || destination.pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT ||
        destination.mipLevels() != 1 || destination.arrayLayers() != 1)
    {
        throw std::invalid_argument("Scene average destination must be output-sized RGBA16F");
    }
    auto values = resolve();
    std::vector<uint32_t> packed(values.size()/2);
    for (size_t i = 0; i < packed.size(); ++i)
    {
        const glm::vec2 pair{values[i*2],values[i*2+1]};
        if (!std::isfinite(pair.x) || !std::isfinite(pair.y) || std::abs(pair.x) > 65504 || std::abs(pair.y) > 65504)
        {
            throw std::overflow_error("Scene average exceeds finite half-float HDR range");
        }
        packed[i] = glm::packHalf2x16(pair);
    }
    destination.upload({packed.data(), packed.size()*sizeof(uint32_t)});
}

uint32_t FrameAccumulator::completedFrames() const
{
    return _completed;
}

bool FrameAccumulator::complete() const
{
    return _requested != 0 && _completed == _requested;
}
}
