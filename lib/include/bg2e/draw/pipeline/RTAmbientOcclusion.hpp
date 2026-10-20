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

#include <bg2e/draw/pipeline/TracingSceneResources.hpp>
#include <bg2e/utils/BlueNoise.hpp>
#include <filesystem>

namespace bg2e::draw::pipeline {
enum class SurfaceProfile : uint32_t
{
    CameraDepth, WorldPositionCoverage
};
enum class RTAOQuality : uint32_t
{
    Low, Medium, High, Ultra
};

// Borrowed invocation images. Camera normals are encoded RGB with zero-clear
// coverage; explicit normals are signed world XYZ, with separate R8 coverage.
struct SurfaceInputs {
    SurfaceProfile profile = SurfaceProfile::CameraDepth;
    gpu::Image* normal = nullptr; // RGBA16 float
    gpu::Image* depthOrPosition = nullptr; // D16/D32 depth or RGBA32 world position
    gpu::Image* coverage = nullptr; // Required R8 UNORM for explicit surfaces
    glm::mat4 inverseViewProjection{1};

    gpu::Size2D extent() const;
    void validate(gpu::Device& device) const;
};
struct RTAOSettings {
    bool enabled = false;
    RTAOQuality quality = RTAOQuality::High;
    uint32_t sampleCount = 8, bounceCount = 2;
    float radius = 0.56f, bias = 0.0017f, falloff = 1, bounceAttenuation = 0.35f;
    bool useBlueNoise = true;
};
struct RTAOConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "draw/rtao"; // camera/world_{hash,blue}_256 templates; exact runtime capacity.
    std::string shaderInterface = "draw.rtao.v2";
    std::shared_ptr<utils::BlueNoise> noise;
};
struct RTAOContext {
    gpu::CommandBuffer& command;
    uint32_t slot = 0;
    uint64_t serial = 0; // Acquired presentation/job identity, not sample sequence.
    uint32_t renderedSample = 0;
    SurfaceInputs surface;
    gpu::Image& output; // R16_SFLOAT sampled/storage/color target; neutral=1.
};

// Public Device-backed pass, independent of a concrete renderer/camera/scene UI.
// Custom native shaders implement draw.rtao.v2; subclass record is replaceable.
// Retain caller surfaces/output and prepared snapshots through GPU completion.
class BG2E_API RTAmbientOcclusion {
  public:
    explicit RTAmbientOcclusion(RTAOConfiguration configuration = {});
    virtual ~RTAmbientOcclusion() = default;
    RTAmbientOcclusion(const RTAmbientOcclusion&) = delete;
    RTAmbientOcclusion& operator=(const RTAmbientOcclusion&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    virtual void prepare(uint32_t slot, uint64_t serial, const RenderQueue& queue);
    virtual void prepareSnapshot(uint32_t slot, uint64_t serial, std::shared_ptr<TracingSnapshot> snapshot);
    virtual void record(const RTAOContext& context);
    virtual void reset(); // Retired/drained boundary, not a temporal-history API.
    virtual void cleanup();

    const RTAOSettings& settings() const;
    void setEnabled(bool value);
    void setQuality(RTAOQuality value);
    void setSampleCount(uint32_t value);
    void setBounceCount(uint32_t value);
    void setRadius(float value);
    void setBias(float value);
    void setFalloff(float value);
    void setBounceAttenuation(float value);
    void setUseBlueNoise(bool value);
    bool enabled() const;
    RTAOQuality quality() const;
    uint32_t sampleCount() const;
    uint32_t bounceCount() const;
    float radius() const;
    float bias() const;
    float falloff() const;
    float bounceAttenuation() const;
    bool useBlueNoise() const;
    void setNoise(std::shared_ptr<utils::BlueNoise> noise);
    std::shared_ptr<utils::BlueNoise> noise() const;
    virtual reflection::TypeInfo settingsType() const;
    virtual void* settingsInstance();
    virtual bool available() const;
    const std::string& status() const;
    uint32_t tableCapacity() const;
    gpu::Size2D outputExtent(gpu::Size2D surface) const;
    std::shared_ptr<TracingSceneResources> tracingResources() const;

  protected:
    struct Invocation {
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<gpu::ResourceSet> surfaceSet, sceneSet;
        std::shared_ptr<gpu::Image> noiseImage;
        std::shared_ptr<gpu::Sampler> noiseSampler;
    };
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false;
        std::shared_ptr<TracingSnapshot> snapshot;
        std::vector<Invocation> invocations;
    };
    RTAOConfiguration _configuration;
    RTAOSettings _settings;
    gpu::Device* _device = nullptr;
    uint32_t _capacity = 0;
    std::shared_ptr<TracingSceneResources> _tracing;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::array<std::shared_ptr<gpu::PipelineLayout>,4> _layouts;
    std::array<std::shared_ptr<gpu::ShaderModule>,4> _shaders;
    std::array<std::shared_ptr<gpu::ComputePipeline>,4> _pipelines;
    std::vector<Slot> _slots;
    std::string _status = "Not initialized";
};
BG2E_API gpu::PipelineLayoutDescription rtaoBindingLayout(uint32_t capacity, bool blueNoise);
}
