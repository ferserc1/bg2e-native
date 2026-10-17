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
#include <bg2e/draw/pipeline/FinalProcessor.hpp>
#include <bg2e/draw/pipeline/FullscreenPass.hpp>
#include <bg2e/gpu/Device.hpp>
#include <array>
#include <vector>

namespace bg2e::draw::pipeline {
struct SMAAConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "draw/smaa", shaderInterface = "draw.smaa.v1";
};
struct SMAAInput {
    gpu::CommandBuffer& command;
    uint32_t slot;
    uint64_t serial;
    gpu::Image& color;
};
struct SMAAResult {
    gpu::Image* image = nullptr;
    std::shared_ptr<gpu::Image> owner;
};
// Orthogonal SMAA 1x: luma edges, official area/search lookups, neighborhood blend.
// The lookup byte headers remain vendored under their original license.
class BG2E_API SMAAProcessor : public FinalProcessor {
  public:
    explicit SMAAProcessor(SMAAConfiguration configuration = {});
    SMAAProcessor(const SMAAProcessor&) = delete;
    SMAAProcessor& operator=(const SMAAProcessor&) = delete;

    void initialize(Engine& engine, gpu::PixelFormat outputFormat, uint32_t slots) override;
    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    // Borrow source/Device through completion. Call once per completed slot/serial;
    // fresh outputs remain valid while returned owner is retained. No submission.
    virtual SMAAResult process(const SMAAInput& input);
    bool record(const FrameContext& frame) override;
    void cleanup() override;

    virtual bool enabled() const;
    virtual void setEnabled(bool value);
    virtual float threshold() const;
    virtual void setThreshold(float value);
    virtual uint32_t searchSteps() const;
    virtual void setSearchSteps(uint32_t value);
    bool available() const override;
    std::string status() const override;
    reflection::TypeInfo settingsType() const override;
    void* settingsInstance() override;

  protected:
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false;
        std::array<std::shared_ptr<gpu::Image>,3> images;
        std::array<std::shared_ptr<gpu::ResourceSet>,3> sets;
        std::shared_ptr<gpu::Buffer> uniforms;
    };
    SMAAConfiguration _configuration;
    bool _enabled = false, _ready = false, _copyReady = false;
    float _threshold = 0.05f;
    uint32_t _searchSteps = 16;
    gpu::Device* _device = nullptr;
    Engine* _engine = nullptr;
    std::array<std::shared_ptr<gpu::ComputePipeline>,3> _pipelines;
    std::array<std::shared_ptr<gpu::ShaderModule>,3> _shaders;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::Sampler> _linear;
    std::shared_ptr<gpu::Image> _area, _search;
    std::vector<Slot> _slots;
    FullscreenPass _copy;
    std::string _status = "Not initialized";
};
}
