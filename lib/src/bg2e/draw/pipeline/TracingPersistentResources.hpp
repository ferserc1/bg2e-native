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

#include <bg2e/draw/pipeline/TracingSceneResources.hpp>
#include "TracingTableLookup.hpp"
#include <mutex>

namespace bg2e::draw::pipeline {
// Immutable native tuple. Only its bounded descriptor cache is mutable.
class TracingResourceTable : public std::enable_shared_from_this<TracingResourceTable> {
  public:
    gpu::Device* device = nullptr;
    uint32_t admissionCapacity = 0;
    std::vector<std::shared_ptr<gpu::Image>> images;
    std::vector<std::shared_ptr<gpu::Sampler>> samplers;
    std::vector<uint64_t> generations;
    std::shared_ptr<gpu::Image> fallbackImage;
    std::shared_ptr<gpu::Sampler> fallbackSampler;
    uint64_t fallbackGeneration = 0;

    bool matches(const TracingResourceTable& other) const;
    void validate() const;
    std::shared_ptr<gpu::ResourceSet> bindings(gpu::PipelineLayout& layout, uint32_t capacity,
        bool shadow, bool reuse) const;

  private:
    struct Binding {
        gpu::PipelineLayout* layout = nullptr;
        std::shared_ptr<gpu::DeviceResource> layoutOwner;
        uint32_t capacity = 0;
        bool shadow = false;
        std::shared_ptr<gpu::ResourceSet> set;
    };
    mutable std::mutex _mutex;
    mutable std::vector<Binding> _bindings;
};

class TracingMaterialGeneration {
  public:
    std::vector<detail::MaterialKey> keys;
    std::vector<std::shared_ptr<MaterialBase>> owners;
    std::vector<TracingMaterialData> materials;
    std::vector<uint32_t> fallbackSlots;
    std::shared_ptr<const TracingResourceTable> resources = std::make_shared<TracingResourceTable>();
    std::shared_ptr<gpu::Buffer> buffer;
};

struct ShadowIdentity {
    glm::uvec4 ranges{0}; // Rebased geometry range and canonical object ID; alpha slot reserved.
    glm::vec4 alpha{0};
    glm::uvec4 options{0};
    detail::ImageKey image;
    const gpu::Sampler* sampler = nullptr;
    const Texture* texture = nullptr;
    uint64_t textureRevision = 0;
    bool operator==(const ShadowIdentity&) const = default;
};

class TracingShadowGeneration {
  public:
    uint32_t capacity = 0;
    std::shared_ptr<const TracingGeometry> geometry;
    std::vector<ShadowIdentity> keys;
    std::vector<std::shared_ptr<Texture>> owners; // Retain even overflowed source identities.
    ShadowQueryTable table;
    std::shared_ptr<const TracingResourceTable> resources = std::make_shared<TracingResourceTable>();
    std::shared_ptr<gpu::Buffer> buffer;
};

struct AccelerationIdentity {
    const Drawable::DrawMesh* mesh = nullptr;
    const gpu::RayTracingMesh* blas = nullptr;
    uint64_t blasGeneration = 0;
    std::string nodeId;
    glm::mat4 transform{1};
    glm::uvec4 ranges{0}; // Packed index/vertex base, triangle count, material address.
    glm::uvec4 identity{0}; // Object/submesh correspondence, mirroring, reserved.
    uint32_t mask = 0;
    gpu::RayTracingOpacity opacity = gpu::RayTracingOpacity::Geometry;
    bool tracingEnabled = false;
    bool operator==(const AccelerationIdentity&) const = default;
};

// Native scene and instance records are immutable once recorded. Completion is
// tied to the exact recording, never to a frame serial or the wrapper's _built.
class TracingAccelerationGeneration {
  public:
    std::shared_ptr<const TracingGeometry> geometry;
    std::vector<std::shared_ptr<gpu::RayTracingMesh>> blasOwners;
    std::vector<AccelerationIdentity> keys;
    std::vector<TracingInstanceData> instances;
    std::shared_ptr<gpu::Buffer> buffer;
    std::shared_ptr<gpu::RayTracingScene> scene;
    std::shared_ptr<const gpu::detail::RecordingCompletion> completion;
    bool recorded = false;

    bool completed() const;
};

}
