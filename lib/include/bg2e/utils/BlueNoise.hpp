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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <span>

namespace bg2e::gpu {
class Device;
class Image;
class Sampler;
}

namespace bg2e::utils {
// Borrowed layer-major, row-major interleaved byte ranks; no graphics setup.
// Keep custom byte owners alive while this view is used. GPU build copies data.
struct BG2E_API BlueNoiseData {
    gpu::Size2D tileSize;
    uint32_t channels = 0;
    uint32_t layers = 0;
    std::span<const uint8_t> bytes;

    static BlueNoiseData embedded();
    void validate() const;

    std::array<uint8_t, 4> texel(int64_t x, int64_t y, uint32_t layer) const;
    glm::vec2 sample2D(int32_t x, int32_t y, uint32_t renderedFrame, uint32_t sample) const;
};

// Null data selects deterministic hash sampling; no GPU resource is required.
// Hash samples are not blue noise. Sequence keys belong to the effect/bake caller.
BG2E_API glm::vec2 sampleNoise2D(const BlueNoiseData* data, int32_t x, int32_t y,
                               uint32_t renderedFrame, uint32_t sample);

// Explicitly shared immutable residency on one borrowed device. Stop producers
// before build/cleanup. Device must outlive this object and all exported owners.
// GPU layout: RGBA8 UNORM, width=tileWidth, height=tileHeight*layers, one mip.
class BG2E_API BlueNoise {
  public:
    explicit BlueNoise(gpu::Device& device);
    ~BlueNoise() = default;
    BlueNoise(const BlueNoise&) = delete;
    BlueNoise& operator=(const BlueNoise&) = delete;

    void build(const BlueNoiseData& data = BlueNoiseData::embedded());
    void cleanup();

    bool isReady() const;
    gpu::Device& device() const;
    gpu::Size2D tileSize() const;
    uint32_t channelCount() const;
    uint32_t layerCount() const;
    // x=tile width, y=tile height, z=source channels, w=logical layers.
    glm::uvec4 shaderMetadata() const;
    const std::shared_ptr<gpu::Image>& image() const;
    const std::shared_ptr<gpu::Sampler>& sampler() const;

  private:
    gpu::Device* _device;
    std::shared_ptr<gpu::Image> _image;
    std::shared_ptr<gpu::Sampler> _sampler;
    glm::uvec4 _metadata{0};
};
}
