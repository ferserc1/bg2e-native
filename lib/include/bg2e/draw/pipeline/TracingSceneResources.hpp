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

// One acquired-slot/explicit job snapshot. Retain through GPU completion, along
// with every resource set made from it. Canonical IDs are never TLAS indices.
class BG2E_API TracingSnapshot {
  public:
    TracingSnapshot() = default;
    TracingSnapshot(const TracingSnapshot&) = delete;
    TracingSnapshot& operator=(const TracingSnapshot&) = delete;

    gpu::Device* device = nullptr; // Borrowed; outlives all owners.
    uint32_t capacity = 0;
    uint64_t fallbackTextureSlots = 0; // Capacity-degraded slots; authored/raster resources are unchanged.
    std::shared_ptr<const TracingGeometry> geometry;
    std::vector<CollectRayTracingInstancesVisitor::Instance> owners;
    std::vector<TracingInstanceData> instances;
    std::vector<TracingMaterialData> materials;
    std::vector<std::shared_ptr<gpu::Image>> images;
    std::vector<std::shared_ptr<gpu::Sampler>> samplers;
    std::shared_ptr<gpu::Buffer> instanceBuffer, materialBuffer;
    std::shared_ptr<gpu::RayTracingScene> scene;
    ShadowQueryTable shadowTable;
    std::shared_ptr<gpu::Buffer> shadowInstances;

    bool empty() const;
    // Once per recording/submission. Discard the snapshot if recording aborts.
    void build(gpu::CommandBuffer& command);
    // Shared, immutable bindings cached per layout/set/capacity for this snapshot
    // only. Consumers must not mutate/clean up the returned set. Calls are serial
    // on the recording thread, like build(). Shared-owned layouts are retained.
    std::shared_ptr<gpu::ResourceSet> resourceSet(gpu::PipelineLayout& layout, uint32_t set = 1,
        uint32_t bindingCapacity = 0) const;

  private:
    struct SceneBindings {
        gpu::PipelineLayout* layout = nullptr;
        std::shared_ptr<gpu::DeviceResource> layoutOwner;
        uint32_t setIndex = 0, capacity = 0;
        std::shared_ptr<gpu::ResourceSet> resources;
    };
    mutable std::vector<SceneBindings> _sceneBindings;
    bool _built = false;
};

// Native map: set=1 bindings 0..3 -> Metal buffers2..5, TLAS4 -> buffer6,
// images5/samplers6 -> Metal argument record buffers8/9, each record id0.
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

  private:
    gpu::Device* _device;
    uint32_t _capacity;
    std::shared_ptr<const TracingGeometry> _geometry;
    bool _capacityWarningActive = false;
};
}
