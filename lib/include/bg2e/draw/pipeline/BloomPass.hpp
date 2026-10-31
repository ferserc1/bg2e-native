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

#pragma once

#include <bg2e/draw/FrameContext.hpp>
#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <filesystem>

namespace bg2e::draw::pipeline {
enum class BloomResolution : uint32_t
{
    Half = 2, Quarter = 4
};

struct BG2E_API BloomSettings {
    bool enabled = true;
    float intensity = 0.08f;
    float threshold = 1.0f; // Scene-linear maximum RGB component, before exposure.
    float scatter = 0.7f;
    uint32_t levels = 6;
    BloomResolution resolution = BloomResolution::Quarter;

    void validate() const;
};

// Single RGBA16F pyramid per acquired slot; no history or full-resolution scratch.
// Upsampling blends into the retained downsample levels without sampling the
// destination attachment. Bindings are reused only after their slot completes.
// cleanup requires drained GPU work, including the final reader of the pyramid.
class BG2E_API BloomPass {
  public:
    void initialize(Engine& engine, const std::filesystem::path& shaderRoot, uint32_t slots);
    gpu::Image* record(const FrameContext& frame, gpu::Image& hdr, const BloomSettings& settings);
    void cleanup();

  private:
    struct Slot {
        std::vector<std::shared_ptr<gpu::Image>> images;
        std::vector<std::shared_ptr<gpu::ResourceSet>> bindings;
        gpu::Size2D sourceExtent;
        uint32_t divisor = 0, levels = 0;
        size_t invocation = 0;
    };
    struct alignas(16) Push {
        glm::vec4 extent{0}; // Destination size, extraction divisor, filter mode.
        glm::vec4 parameters{0}; // Threshold, scatter.
    };
    static_assert(sizeof(Push) == 32);

    void draw(const FrameContext& frame, Slot& slot, gpu::Image& source,
        gpu::Image& destination, const Push& push, bool upsample);

    Engine* _engine = nullptr;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::ShaderModule> _vertex, _fragment;
    std::shared_ptr<gpu::GraphicsPipeline> _downsample, _upsample;
    std::shared_ptr<gpu::Sampler> _sampler;
    std::vector<Slot> _slots;
};
}
