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
#include <bg2e/reflection/TypeInfo.hpp>
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/draw/environment/EnvironmentResources.hpp>
#include <bg2e/gpu/RayTracingPipeline.hpp>

namespace bg2e::draw::pipeline {
using RTGIQuality = RTAOQuality;
struct RTGISettings {
    bool enabled = false, useBlueNoise = true, useShadows = true;
    RTGIQuality quality = RTGIQuality::High;
    uint32_t sampleCount = 4, bounceCount = 2, shadowSamples = 1;
    float bias = 0.0017f, maxDistance = 100;
};
struct RTGIConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "draw/rtgi", shaderInterface = "draw.rtgi.v1";
    std::shared_ptr<utils::BlueNoise> noise;
};
struct RTGIContext {
    gpu::CommandBuffer& command;
    uint32_t slot;
    uint64_t serial;
    uint32_t renderedSample;
    SurfaceInputs surface;
    gpu::Image& output; // RGBA16 float: linear diffuse incoming radiance/pi, alpha validity.
    LightUniforms lights;
    std::shared_ptr<const environment::EnvironmentResources> environment;
};
class BG2E_API RTGlobalIllumination {
  public:
    explicit RTGlobalIllumination(RTGIConfiguration configuration = {});
    virtual ~RTGlobalIllumination() = default;
    RTGlobalIllumination(const RTGlobalIllumination&) = delete;
    RTGlobalIllumination& operator=(const RTGlobalIllumination&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    virtual void prepare(uint32_t slot, uint64_t serial, const RenderQueue& queue);
    virtual void prepareSnapshot(uint32_t slot, uint64_t serial, std::shared_ptr<TracingSnapshot> snapshot);
    virtual void record(const RTGIContext& context);
    virtual void reset();
    virtual void cleanup();

    const RTGISettings& settings() const;
    bool enabled() const;
    void setEnabled(bool value);
    RTGIQuality quality() const;
    void setQuality(RTGIQuality value);
    uint32_t sampleCount() const;
    void setSampleCount(uint32_t value);
    uint32_t bounceCount() const;
    void setBounceCount(uint32_t value);
    uint32_t shadowSamples() const;
    void setShadowSamples(uint32_t value);
    float bias() const;
    void setBias(float value);
    float maxDistance() const;
    void setMaxDistance(float value);
    bool useBlueNoise() const;
    void setUseBlueNoise(bool value);
    bool useShadows() const;
    void setUseShadows(bool value);
    void setNoise(std::shared_ptr<utils::BlueNoise> noise);
    std::shared_ptr<utils::BlueNoise> noise() const;
    virtual reflection::TypeInfo settingsType() const;
    virtual void* settingsInstance();
    virtual bool available() const;
    const std::string& status() const;
    uint32_t tableCapacity() const;
    virtual gpu::Size2D outputExtent(gpu::Size2D surface) const;
    std::shared_ptr<TracingSceneResources> tracingResources() const;

  protected:
    struct Invocation {
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<gpu::ResourceSet> surfaceSet, sceneSet;
        std::shared_ptr<gpu::Image> noiseImage;
        std::shared_ptr<gpu::Sampler> noiseSampler;
        std::shared_ptr<const environment::EnvironmentResources> environment;
    };
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false;
        std::shared_ptr<TracingSnapshot> snapshot;
        std::vector<Invocation> invocations;
    };
    RTGIConfiguration _configuration;
    RTGISettings _settings;
    gpu::Device* _device = nullptr;
    gpu::BackendType _backend{};
    uint32_t _capacity = 0;
    std::shared_ptr<TracingSceneResources> _tracing;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::array<std::shared_ptr<gpu::PipelineLayout>,4> _layouts;
    std::array<std::shared_ptr<gpu::ShaderModule>,4> _shaders;
    std::array<std::shared_ptr<gpu::ShaderModule>,12> _stages;
    std::array<std::shared_ptr<gpu::ComputePipeline>,4> _compute;
    std::array<std::shared_ptr<gpu::RayTracingPipeline>,4> _pipelines;
    std::vector<Slot> _slots;
    std::string _status = "Not initialized";
};
BG2E_API gpu::PipelineLayoutDescription rtgiBindingLayout(uint32_t capacity, bool blueNoise, gpu::BackendType backend);
}
