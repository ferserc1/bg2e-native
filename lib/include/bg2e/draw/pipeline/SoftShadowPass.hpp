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
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>

#include <bg2e/draw/pipeline/RenderLayer.hpp>
#include <bg2e/draw/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/gpu/ComputePipeline.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <filesystem>
#include <stdexcept>

namespace bg2e::draw::pipeline {
class TracingSnapshot;
class TracingSceneResources;
inline constexpr uint32_t MaxShadowAlphaTextures = gpu::MaxSampledResourceTableCapacity;
inline constexpr uint32_t MaxShadowSamples = 64;
struct SoftShadowSettings {
    bool enabled = true;
    float bias = 0.01f;
    float directionalDistance = 1000000.0f;
    // Per-light shadowSamples is capped to this explicit quality budget.
    uint32_t maxSamples = 16;
};
// Dense TLAS-instance order; ranges.w retains the original frame object ID.
// Indexed UVs are fetched from shared tracing geometry on the GPU.
struct alignas(16) ShadowInstanceData {
    glm::uvec4 ranges{0}; // first index, triangle count, alpha slot or UINT_MAX, objectId
    glm::vec4 alpha{1}; // scale.xy, cutoff, albedo alpha multiplier
    glm::uvec4 options{0}; // albedo UV set, material flags, vertex base, reserved
};
struct ShadowQueryTable {
    std::vector<ShadowInstanceData> instances;
    std::vector<std::shared_ptr<Texture>> alphaTextures;
    uint64_t fallbackTextureSlots = 0;
};
// Builders return mesh-local index ranges (vertex base zero). Snapshot preparation
// rebases them to its immutable shared geometry without copying any triangles.
BG2E_API ShadowQueryTable buildShadowQueryTable(const CollectRayTracingInstancesVisitor& collection,
    uint32_t capacity = MaxShadowAlphaTextures);
BG2E_API ShadowQueryTable buildShadowQueryTable(const std::vector<CollectRayTracingInstancesVisitor::Instance>& instances,
    uint32_t capacity);
BG2E_API PassContract softShadowContract(gpu::PixelFormat depth);
BG2E_API gpu::PipelineLayoutDescription softShadowBindingLayout(uint32_t capacity = MaxShadowAlphaTextures);
static_assert(sizeof(ShadowInstanceData) == 48);
static_assert(offsetof(ShadowInstanceData, alpha) == 16 && offsetof(ShadowInstanceData, options) == 32);

// Public compute-based visibility pass. No render-stage query capability or RT
// pipeline/SBT is needed. Unsupported creation reports status and allocates no TLAS.
class BG2E_API SoftShadowPass : public RenderLayer {
  public:
    explicit SoftShadowPass(std::filesystem::path shaderRoot = {}, uint32_t slots = 0,
        gpu::PixelFormat depth = gpu::PixelFormat::D32_SFLOAT);

    SoftShadowSettings& settings();
    const SoftShadowSettings& settings() const;
    bool available() const;
    bool sceneSupported() const;
    uint32_t alphaTextureCapacity() const;
    const std::string& status() const;
    const PassContract& contract() const override;

    void initialize(Engine& engine, const LayerConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node>) override;
    void resize(const LayerConfiguration& configuration) override;
    void update(const FrameContext&) override;

    // After acquiring the completed slot, once per presentation, before consumers.
    // A renderer snapshot supplies the common TLAS, geometry and shadow metadata;
    // omit it for independent standalone preparation.
    void prepare(const FrameContext& frame, const RenderQueue& queue,
        const glm::mat4& view, const glm::mat4& projection,
        std::shared_ptr<TracingSnapshot> snapshot = nullptr);
    void record(const PassContext& context) override;
    void cleanup() override;

    // Diagnostic fallback: construct fresh scene and alpha bindings for every invocation.
    void setBindingReuseEnabled(bool enabled)
    {
        _bindingReuseEnabled = enabled;
    }

    TemporaryPoolStatistics poolStatistics() const;

    // Diagnostic baseline: allocate fresh invocation/image storage.
    void setPoolingEnabled(bool enabled)
    {
        _poolingEnabled = enabled;
    }

  protected:
    struct Slot {
        std::shared_ptr<TracingSnapshot> snapshot;
        std::shared_ptr<gpu::Buffer> uniforms;
        struct Invocation {
            std::shared_ptr<TemporaryBindings> pooled;
            std::shared_ptr<gpu::ResourceSet> stable, resources, alpha;
            std::vector<std::shared_ptr<gpu::DeviceResource>> owners;
        };
        std::vector<Invocation> sets;
        uint64_t serial = 0;
        bool prepared = false;
        bool active = false;
        bool built = false;
    };
    std::filesystem::path _shaderRoot;
    uint32_t _requestedSlots;
    SoftShadowSettings _settings;
    Engine* _engine = nullptr;
    std::shared_ptr<TemporaryResourcePool> _pool;
    bool _poolingEnabled = true;
    PassContract _contract;
    std::string _status = "Not initialized";
    uint32_t _alphaCapacity = 0;
    bool _capacityWarningActive = false;
    bool _bindingReuseEnabled = true;
    std::shared_ptr<gpu::ShaderModule> _shader;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::ComputePipeline> _pipeline;
    std::shared_ptr<gpu::Sampler> _nearest;
    std::vector<Slot> _slots;
    std::shared_ptr<TracingSceneResources> _standaloneResources;
};
}
