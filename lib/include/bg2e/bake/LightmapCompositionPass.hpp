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

#include <bg2e/bake/UvTemporalAccumulator.hpp>
#include <bg2e/bake/LightmapPixels.hpp>

namespace bg2e::bake {
struct CompositionConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "bake/composition", dilationPrefix = "bake/dilation";
    std::string shaderInterface = "bake.composition.v1";
};
struct CompositionResult {
    std::shared_ptr<gpu::Image> image; // RGBA32F multiplier, alpha one, v=0 top.
    uint64_t sourceGeneration = 0, targetGeneration = 0;
    LightmapMode mode = LightmapMode::RTGI;
};
// Normalizes exact raw averages, then extends nearest original chart texels into
// uncovered padding only. No cross-chart averaging; original interiors are fixed.
class BG2E_API LightmapCompositionPass {
  public:
    explicit LightmapCompositionPass(CompositionConfiguration configuration = {});
    virtual ~LightmapCompositionPass();
    LightmapCompositionPass(const LightmapCompositionPass&) = delete;
    LightmapCompositionPass& operator=(const LightmapCompositionPass&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    virtual CompositionResult record(gpu::CommandBuffer& command, uint32_t slot, uint64_t serial,
        const AccumulationResult& accumulation, const LightmapSettings& settings,
        std::shared_ptr<const draw::environment::EnvironmentResources> environment);
    virtual void cleanup();

    static LightmapPixels convertRgba32(gpu::Size2D extent, const std::vector<float>& rgba,
        LightmapMode mode, LightmapPixelFormat format, float exposureEV);

    // Completed output only: finite nonnegative HDR validation precedes CPU byte
    // conversion. RGB8: linear UNORM, GI exposure only. RGB32F: no exposure/clamp.
    static LightmapPixels readPixels(const CompositionResult& result, const LightmapSettings& settings);

  protected:
    struct Slot {
        uint64_t serial = 0;
        bool used = false;
        std::vector<std::shared_ptr<gpu::Image>> images;
        std::vector<std::shared_ptr<gpu::ResourceSet>> sets;
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<const draw::environment::EnvironmentResources> environment;
    };
    CompositionConfiguration _configuration;
    gpu::Device* _device = nullptr;
    std::array<std::shared_ptr<gpu::PipelineLayout>,4> _layouts;
    std::array<std::shared_ptr<gpu::ComputePipeline>,4> _pipelines;
    std::array<std::shared_ptr<gpu::ShaderModule>,4> _shaders;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::vector<Slot> _slots;
};
}
