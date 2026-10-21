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
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/reflection/TypeInfo.hpp>
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/draw/environment/EnvironmentResources.hpp>
#include <bg2e/gpu/RayTracingPipeline.hpp>

namespace bg2e::draw::pipeline {
using RTReflectionQuality = RTAOQuality;
struct RTReflectionSettings {
    bool enabled = true, useBlueNoise = true, useShadows = true;
    RTReflectionQuality quality = RTReflectionQuality::High;
    uint32_t sampleCount = 6, shadowSamples = 1;
    float bias = 0.02f, maxDistance = 50, maxRoughness = 0.75f, roughnessSpread = 1;
};
struct RTReflectionConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "draw/rt_reflections", shaderInterface = "draw.rt_reflections.v3";
    std::shared_ptr<utils::BlueNoise> noise;
};
struct RTReflectionContext {
    gpu::CommandBuffer& command;
    uint32_t slot;
    uint64_t serial;
    uint32_t renderedSample;
    SurfaceInputs surface;
    gpu::Image& output; // RGBA16 float: unweighted hit radiance, alpha=accepted hits/samples.
    gpu::Image& material; // Sampled RGBA8 UNORM or RGBA16 float, G=receiver roughness.
    gpu::Size2D displayExtent; // Quality is relative to display, not surface/G-buffer.
    glm::vec3 cameraPosition{0};
    LightUniforms lights;
    std::shared_ptr<const environment::EnvironmentResources> environment;
};
class BG2E_API RTReflections {
  public:
    explicit RTReflections(RTReflectionConfiguration configuration = {});
    virtual ~RTReflections() = default;
    RTReflections(const RTReflections&) = delete;
    RTReflections& operator=(const RTReflections&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    virtual void prepare(uint32_t slot, uint64_t serial, const RenderQueue& queue);
    virtual void prepareSnapshot(uint32_t slot, uint64_t serial, std::shared_ptr<TracingSnapshot> snapshot);
    virtual void record(const RTReflectionContext& context);
    virtual void reset();
    virtual void cleanup();

    const RTReflectionSettings& settings() const;
    bool enabled() const;
    void setEnabled(bool value);
    RTReflectionQuality quality() const;
    void setQuality(RTReflectionQuality value);
    uint32_t sampleCount() const;
    void setSampleCount(uint32_t value);
    float maxRoughness() const;
    void setMaxRoughness(float value);
    float roughnessSpread() const;
    void setRoughnessSpread(float value);
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
    virtual gpu::Size2D outputExtent(gpu::Size2D display) const;
    std::shared_ptr<TracingSceneResources> tracingResources() const;

    TemporaryPoolStatistics poolStatistics() const;

    // Diagnostic baseline: allocate fresh invocation/image storage.
    void setPoolingEnabled(bool enabled)
    {
        _poolingEnabled = enabled;
    }

  protected:
    struct Invocation {
        std::shared_ptr<TemporaryBindings> pooled;
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<gpu::ResourceSet> surfaceSet, sceneSet, tableSet;
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
    RTReflectionConfiguration _configuration;
    RTReflectionSettings _settings;
    gpu::Device* _device = nullptr;
    std::shared_ptr<TemporaryResourcePool> _pool;
    bool _poolingEnabled = true;
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
BG2E_API gpu::PipelineLayoutDescription rtReflectionBindingLayout(uint32_t capacity, bool blueNoise, gpu::BackendType backend);
}
