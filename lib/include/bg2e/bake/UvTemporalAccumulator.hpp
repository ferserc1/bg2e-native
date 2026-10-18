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

#include <bg2e/bake/LightmapBakeExecutor.hpp>
#include <bg2e/gpu/ComputePipeline.hpp>

namespace bg2e::bake {
struct AccumulationConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "bake/accumulation", shaderInterface = "bake.accumulation.v1";
};
struct AccumulationResult {
    std::shared_ptr<gpu::Image> sums; // RGBA32F: raw RGB sums, A=sticky numeric error.
    std::shared_ptr<gpu::Image> counts; // R32_UINT exact valid update count.
    UvSurfaceResult surface;
    LightmapMode mode = LightmapMode::RTGI;
    uint32_t updates = 0;
};
// Fresh outputs per record: discard never mutates committed history. Not a rolling
// display/denoise history. Retain input/output owners through completion.
class BG2E_API UvTemporalAccumulator {
  public:
    explicit UvTemporalAccumulator(AccumulationConfiguration configuration = {});
    virtual ~UvTemporalAccumulator();
    UvTemporalAccumulator(const UvTemporalAccumulator&) = delete;
    UvTemporalAccumulator& operator=(const UvTemporalAccumulator&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    virtual std::shared_ptr<const AccumulationResult> record(gpu::CommandBuffer& command,
        uint32_t slot, uint64_t serial, const BakeSample& sample,
        std::shared_ptr<const AccumulationResult> completedPrevious = nullptr);
    virtual void cleanup();

    // Completed GPU work only. Rejects missing coverage, invalid/saturated signal,
    // overflow and any covered texel without every requested valid sample.
    static void validateCompleted(const AccumulationResult& result, uint32_t requestedUpdates);

  protected:
    struct Slot {
        uint64_t serial = 0;
        bool used = false;
        std::shared_ptr<AccumulationResult> result;
        std::shared_ptr<gpu::ResourceSet> set;
        std::shared_ptr<gpu::Buffer> uniforms;
        std::vector<std::shared_ptr<gpu::Image>> inputs;
    };
    AccumulationConfiguration _configuration;
    gpu::Device* _device = nullptr;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::ComputePipeline> _pipeline;
    std::shared_ptr<gpu::ShaderModule> _shader;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::vector<Slot> _slots;
};
}
