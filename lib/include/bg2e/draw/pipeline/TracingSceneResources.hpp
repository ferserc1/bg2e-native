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

#include <bg2e/draw/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/draw/pipeline/SoftShadowPass.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ResourceSet.hpp>

namespace bg2e::draw::pipeline {
struct alignas(16) TracingVertex {
    glm::vec4 position{0}, normal{0}, uv{0}, tangent{0};
};
struct alignas(16) TracingInstanceData {
    glm::mat4 model{1}, normal{1};
    glm::uvec4 ranges{0}; // index base, vertex base, triangle count, material index
    glm::uvec4 identity{0}; // canonical object ID, submesh, mirrored transform, reserved
};
struct alignas(16) TracingMaterialData {
    MaterialData values;
    std::array<glm::uvec4,2> images{}, samplers{}; // Six slots; UINT32_MAX in both means neutral/no texture.
};
static_assert(sizeof(TracingVertex) == 64 && sizeof(TracingInstanceData) == 160 && sizeof(TracingMaterialData) == 256);
static_assert(offsetof(TracingVertex, uv) == 32 && offsetof(TracingVertex, tangent) == 48);
static_assert(offsetof(TracingMaterialData, images) == 192 && offsetof(TracingMaterialData, samplers) == 224);

// Immutable packed geometry shared across completed-slot snapshots. Mesh CPU/GPU
// data is immutable after build; reload creates a new DrawMesh identity.
struct TracingGeometry {
    std::vector<std::shared_ptr<Drawable::DrawMesh>> owners;
    std::vector<TracingVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<glm::uvec2> bases; // Per unique mesh: vertex/index base.
    std::shared_ptr<gpu::Buffer> vertexBuffer, indexBuffer;
};

class TracingMaterialGeneration;
class TracingShadowGeneration;
class TracingAccelerationGeneration;

// One acquired-slot/explicit job snapshot. Retain through GPU completion, along
// with every resource set made from it. Canonical IDs are never TLAS indices.
class BG2E_API TracingSnapshot {
  private:
    std::shared_ptr<const TracingMaterialGeneration> _materialGeneration;
    std::shared_ptr<const TracingShadowGeneration> _shadowGeneration;
    std::shared_ptr<TracingAccelerationGeneration> _accelerationGeneration;

  public:
    TracingSnapshot();
    TracingSnapshot(std::shared_ptr<const TracingMaterialGeneration> materials,
        std::shared_ptr<const TracingShadowGeneration> shadows,
        std::shared_ptr<TracingAccelerationGeneration> acceleration = nullptr);
    TracingSnapshot(const TracingSnapshot&) = delete;
    TracingSnapshot& operator=(const TracingSnapshot&) = delete;

    gpu::Device* device = nullptr; // Borrowed; outlives all owners.
    uint32_t capacity = 0;
    uint64_t fallbackTextureSlots = 0; // Degraded slots counted per instance reference; raster resources are unchanged.
    std::shared_ptr<const TracingGeometry> geometry;
    std::vector<CollectRayTracingInstancesVisitor::Instance> owners;
    const std::vector<TracingInstanceData>& instances;
    const std::vector<TracingMaterialData>& materials; // First encounter of each material identity/effective revision.
    const std::vector<std::shared_ptr<gpu::Image>>& images;
    const std::vector<std::shared_ptr<gpu::Sampler>>& samplers;
    const std::shared_ptr<gpu::Buffer>& instanceBuffer;
    const std::shared_ptr<gpu::Buffer>& materialBuffer;
    const std::shared_ptr<gpu::RayTracingScene>& scene;
    const ShadowQueryTable& shadowTable;
    // Native owners are captured once, independently of mutable Texture wrappers.
    const std::vector<std::shared_ptr<gpu::Image>>& shadowImages;
    const std::vector<std::shared_ptr<gpu::Sampler>>& shadowSamplers;
    const std::vector<uint64_t>& shadowImageGenerations;
    const std::shared_ptr<gpu::Image>& shadowFallbackImage;
    const std::shared_ptr<gpu::Sampler>& shadowFallbackSampler;
    const uint64_t& shadowFallbackGeneration;
    const std::shared_ptr<gpu::Buffer>& shadowInstances;

    bool empty() const;
    // Transitions once per wrapper; builds only a new acceleration generation.
    // Recorded work is not reusable until its submission completes. Discard the
    // snapshot if recording aborts.
    void build(gpu::CommandBuffer& command);
    // Shared, immutable bindings cached per layout/set/capacity for this snapshot
    // only. Consumers must not mutate/clean up the returned set. Calls are serial
    // on the recording thread, like build(). Shared-owned layouts are retained.
    std::shared_ptr<gpu::ResourceSet> resourceSet(gpu::PipelineLayout& layout, uint32_t set = 1,
        uint32_t bindingCapacity = 0) const;

    // Persistent arrays live in set 2, independently of frame-specific set 1.
    std::shared_ptr<gpu::ResourceSet> materialResourceSet(gpu::PipelineLayout& layout,
        uint32_t bindingCapacity = 0, bool reuse = true) const;
    std::shared_ptr<gpu::ResourceSet> shadowAlphaResourceSet(std::shared_ptr<gpu::PipelineLayout> layout,
        uint32_t bindingCapacity, bool reuse = true) const;

    // v5: scene/geometry/metadata set, independent of invocation and alpha sets.
    // Discard the snapshot after native recreation or aborted recording.
    std::shared_ptr<gpu::ResourceSet> shadowResourceSet(std::shared_ptr<gpu::PipelineLayout> layout,
        uint32_t bindingCapacity, uint32_t setIndex = 1, bool reuse = true) const;

  private:
    struct SceneBindings {
        gpu::PipelineLayout* layout = nullptr;
        std::shared_ptr<gpu::DeviceResource> layoutOwner;
        uint32_t setIndex = 0, capacity = 0;
        std::shared_ptr<gpu::ResourceSet> resources;
    };
    mutable std::vector<SceneBindings> _sceneBindings;
    mutable std::vector<SceneBindings> _shadowBindings;
    mutable std::vector<SceneBindings> _uncachedShadowBindings; // Diagnostic invocations retain layouts too.
    bool _built = false;
};

// Native map: set=1 bindings 0..3 -> Metal buffers2..5, TLAS4 -> buffer6;
// set=2 images5/samplers6 -> Metal argument record buffers8/9, each record id0.
BG2E_API gpu::PipelineLayoutDescription tracingBindingLayout(uint32_t capacity,
    gpu::ShaderStage stage = gpu::ShaderStage::Compute,
    const std::vector<gpu::ShaderStage>& additionalStages = {});

class BG2E_API TracingSceneResources {
  public:
    explicit TracingSceneResources(gpu::Device& device, uint32_t capacity);

    // Call only for a retired acquired slot or stopped job. Empty scenes allocate
    // no TLAS/table buffers. Blended transparent geometry is excluded by default.
    // Excess texture/sampler pairs use neutral shader values with a warning.
    // Nonzero shadowCapacity builds shadow metadata from the same collection;
    // UVs remain in the immutable shared geometry buffers.
    std::shared_ptr<TracingSnapshot> prepare(const RenderQueue& queue, uint32_t shadowCapacity = 0);
    void clear(); // Drop cache ownership only; retained snapshots remain valid.
    uint32_t capacity() const;

    // Diagnostic baseline: rebuild immutable material/shadow generations each frame.
    void setPersistenceEnabled(bool enabled)
    {
        _persistenceEnabled = enabled;
    }

    // Diagnostic baseline: new instance storage and full TLAS build per snapshot.
    void setAccelerationReuseEnabled(bool enabled)
    {
        _accelerationReuseEnabled = enabled;
    }

  private:
    gpu::Device* _device;
    uint32_t _capacity;
    std::shared_ptr<const TracingGeometry> _geometry;
    std::shared_ptr<const TracingMaterialGeneration> _materials;
    std::shared_ptr<const TracingShadowGeneration> _shadows;
    // Bounded candidates: recorded/pending entries are never reused prematurely.
    std::vector<std::shared_ptr<TracingAccelerationGeneration>> _accelerations;
    bool _accelerationReuseEnabled = true;
    bool _persistenceEnabled = true;
    bool _capacityWarningActive = false;
};
}
