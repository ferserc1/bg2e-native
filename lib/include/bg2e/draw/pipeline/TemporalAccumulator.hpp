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
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/draw/pipeline/MotionVectorGenerator.hpp>
#include <map>
#include <tuple>

namespace bg2e::draw::pipeline {
enum class TemporalSignal : uint32_t
{
    Visibility, DiffuseRadiance, ReflectedRadiance
};
struct TemporalKey {
    TemporalSignal signal = TemporalSignal::Visibility;
    uint64_t view = 0, layer = 0;

    bool operator<(const TemporalKey& other) const
    {
        return std::tie(signal,view,layer) < std::tie(other.signal,other.view,other.layer);
    }
};
enum class TemporalDiagnosticView : uint32_t
{
    Off, Tokens, LocalHistory, Motion, PreviousPosition, PreviousNormal
};
struct TemporalSettings {
    bool enabled = true, freezeAtLimit = false;
    uint32_t maxFrames = 32;
    float absolutePositionThreshold = 0.02f, relativePositionThreshold = 0.01f, normalThreshold = 0.9f;
};
struct TemporalConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "draw/temporal_accumulation", shaderInterface = "draw.temporal.v1";
    uint32_t maxHistories = 256;
    TemporaryPoolLimits poolLimits;
    std::string metadataShaderInterface; // Optional draw.temporal.v2; v1 stays unchanged.
    std::string metadataShaderPrefix = "draw/temporal_accumulation_v2";
    std::string reprojectionShaderInterface; // Optional draw.temporal.v3; requires v2 metadata support.
    std::string reprojectionShaderPrefix = "draw/temporal_accumulation_v3";
};
struct TemporalSurfaceMetadata {
    std::shared_ptr<gpu::Image> token, depth; // Both supplied or both absent.
    // Optional matching retained RGBA32F position / RGBA16F normal, in previous world space.
    std::shared_ptr<gpu::Image> previousPosition, previousNormal;
    bool legacyCompatible = true; // False: a token-only consumer must use Phase 0 fallback.
    std::shared_ptr<gpu::Image> motion; // Optional retained RG16F, for diagnostics only.
};
// Public borrowed signal/surface profile; independent of private renderer targets.
struct TemporalInput {
    gpu::CommandBuffer& command;
    uint32_t slot;
    uint64_t serial, sceneFrameNumber, generation;
    TemporalKey key;
    gpu::Image& signal;
    SurfaceInputs surface;
    TemporalView view;
    gpu::Image* material = nullptr; // Required for reflections: G=roughness.
    float maxRoughness = 1;
    TemporalSurfaceMetadata metadata;
    bool diagnoseAcceptance = false; // Optional built-in diagnostic kernel; no change to history samples.
};
struct TemporalResult {
    gpu::Image* image = nullptr;
    std::shared_ptr<gpu::Image> owner; // Empty for disabled/unavailable pass-through.
    uint32_t frameCount = 1; // Bounded nominal consecutive rendered-view count.
};
class BG2E_API TemporalAccumulator {
  public:
    explicit TemporalAccumulator(TemporalConfiguration configuration = {});
    virtual ~TemporalAccumulator() = default;
    TemporalAccumulator(const TemporalAccumulator&) = delete;
    TemporalAccumulator& operator=(const TemporalAccumulator&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    // Caller has completed this acquired slot. Only actual scene work calls this.
    // Motion temporarily overrides freezing without changing authored settings.
    // Custom overrides must accept the motion argument and forward it to the base
    // when using its record(). Existing three-argument callers default to no motion.
    virtual void beginFrame(uint32_t slot, uint64_t serial, uint64_t sceneFrameNumber, bool sceneMotion = false);
    virtual TemporalResult record(const TemporalInput& input);
    // GPU-only diagnostic at receiver extent. Empty if optional support/metadata is absent.
    // LocalHistory: red restart/first sample, green accepted, black uncovered,
    // amber when this key has no acceptance diagnostic in the current scene.
    std::shared_ptr<gpu::Image> recordDiagnostic(gpu::CommandBuffer& command, uint32_t slot, uint64_t serial,
        const TemporalKey& key, const TemporalSurfaceMetadata& metadata, TemporalDiagnosticView view);
    bool diagnosticsAvailable() const;
    bool surfaceDiagnosticsAvailable() const;

    virtual void invalidate(); // Logical reset; keeps submitted invocation owners alive.
    virtual void cleanup(); // Drains before releasing all owners.

    const TemporalSettings& settings() const;
    void setEnabled(bool value);
    void setMaxFrames(uint32_t value);
    void setFreezeAtLimit(bool value);
    void setPositionThresholds(float absolute, float relative);
    void setNormalThreshold(float value);
    virtual reflection::TypeInfo settingsType() const;
    virtual void* settingsInstance();
    virtual bool available() const;
    const std::string& status() const;
    virtual bool surfaceMetadataAvailable() const;
    const std::string& surfaceMetadataStatus() const;
    virtual bool surfaceReprojectionAvailable() const;
    const std::string& surfaceReprojectionStatus() const;
    uint64_t generation() const;
    uint32_t frameCount(const TemporalKey& key) const;

    TemporaryPoolStatistics poolStatistics() const;

    // Diagnostic baseline: allocate fresh invocation/image storage.
    void setPoolingEnabled(bool enabled)
    {
        _poolingEnabled = enabled;
    }

  protected:
    struct History {
        std::shared_ptr<const gpu::detail::RecordingCompletion> recording;
        std::shared_ptr<gpu::Image> signal, position, normal;
        TemporalSurfaceMetadata metadata;
        std::shared_ptr<gpu::Image> acceptance;
        TemporalView view;
        uint64_t sequence = 0, generation = 0, reset = 0;
        uint32_t count = 1;
        uint32_t metadataMode = 0; // 0=v1, 1=token-only v2, 2=rigid camera-depth v3.
        uint32_t rollingFramesRemaining = 0; // Subsequent consecutive scene records allowed to roll.
        SurfaceProfile profile{};
        gpu::Size2D surfaceExtent;
        float maxRoughness = 1;
    };
    struct Invocation {
        std::shared_ptr<TemporaryBindings> pooled;
        std::shared_ptr<History> previous, next;
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<gpu::ResourceSet> resources;
        std::shared_ptr<gpu::Image> diagnostic;
        TemporalSurfaceMetadata diagnosticMetadata;
    };
    struct Slot {
        uint64_t serial = 0, sequence = 0;
        bool prepared = false;
        bool sceneMotion = false;
        std::vector<Invocation> invocations;
    };
    TemporalConfiguration _configuration;
    TemporalSettings _settings;
    gpu::Device* _device = nullptr;
    std::shared_ptr<TemporaryResourcePool> _pool;
    bool _poolingEnabled = true;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::shared_ptr<gpu::Image> _zeroPosition, _zeroNormal;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::array<std::shared_ptr<gpu::ShaderModule>,4> _shaders;
    std::array<std::shared_ptr<gpu::ComputePipeline>,4> _pipelines;
    std::shared_ptr<gpu::PipelineLayout> _metadataLayout;
    std::array<std::shared_ptr<gpu::ShaderModule>,4> _metadataShaders;
    std::array<std::shared_ptr<gpu::ComputePipeline>,4> _metadataPipelines;
    std::shared_ptr<gpu::PipelineLayout> _acceptanceLayout;
    std::array<std::shared_ptr<gpu::ShaderModule>,4> _acceptanceShaders;
    std::array<std::shared_ptr<gpu::ComputePipeline>,4> _acceptancePipelines;
    std::shared_ptr<gpu::PipelineLayout> _reprojectionLayout, _reprojectionAcceptanceLayout;
    std::array<std::shared_ptr<gpu::ShaderModule>,2> _reprojectionShaders, _reprojectionAcceptanceShaders;
    std::array<std::shared_ptr<gpu::ComputePipeline>,2> _reprojectionPipelines, _reprojectionAcceptancePipelines;
    std::string _reprojectionStatus = "Optional reprojection interface not requested";
    std::shared_ptr<gpu::PipelineLayout> _diagnosticLayout;
    std::shared_ptr<gpu::ShaderModule> _diagnosticShader, _surfaceDiagnosticShader;
    std::shared_ptr<gpu::ComputePipeline> _diagnosticPipeline, _surfaceDiagnosticPipeline;
    std::string _metadataStatus = "Optional metadata interface not requested";
    std::map<TemporalKey,std::shared_ptr<History>> _histories;
    std::vector<Slot> _slots;
    uint64_t _generation = 1, _latestSequence = 0;
    bool _hasSequence = false;
    std::string _status = "Not initialized";
};
}
