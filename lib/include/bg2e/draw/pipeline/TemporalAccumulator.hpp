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
    virtual void beginFrame(uint32_t slot, uint64_t serial, uint64_t sceneFrameNumber);
    virtual TemporalResult record(const TemporalInput& input);
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
        TemporalView view;
        uint64_t sequence = 0, generation = 0, reset = 0;
        uint32_t count = 1;
        SurfaceProfile profile{};
        gpu::Size2D surfaceExtent;
        float maxRoughness = 1;
    };
    struct Invocation {
        std::shared_ptr<TemporaryBindings> pooled;
        std::shared_ptr<History> previous, next;
        std::shared_ptr<gpu::Buffer> uniforms;
        std::shared_ptr<gpu::ResourceSet> resources;
    };
    struct Slot {
        uint64_t serial = 0, sequence = 0;
        bool prepared = false;
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
    std::map<TemporalKey,std::shared_ptr<History>> _histories;
    std::vector<Slot> _slots;
    uint64_t _generation = 1, _latestSequence = 0;
    bool _hasSequence = false;
    std::string _status = "Not initialized";
};
}
