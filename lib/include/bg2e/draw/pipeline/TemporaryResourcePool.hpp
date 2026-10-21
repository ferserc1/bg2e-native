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

#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/PipelineLayout.hpp>
#include <bg2e/gpu/Buffer.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <vector>
#include <array>

namespace bg2e::draw::pipeline {
struct TemporaryPoolLimits {
    uint64_t retainedBytes = 512ull * 1024 * 1024;
    uint32_t images = 256, bindings = 256, idleFrames = 8;
};

struct TemporaryPoolStatistics {
    uint64_t estimatedBytes = 0;
    size_t images = 0, bindings = 0;
};

// One invocation lease. Do not share mutable leases between invocations. The
// pool may reclaim storage only after this lease is released AND work retires.
class BG2E_API TemporaryBindings {
  public:
    const std::shared_ptr<gpu::ResourceSet>& resources() const;
    const std::shared_ptr<gpu::Buffer>& uniforms() const;
    void writeUniforms(const void* data, uint64_t bytes);
    void setUniformBuffer(gpu::ShaderBinding binding, gpu::Buffer* buffer);
    void setSampledImage(gpu::ShaderBinding binding, gpu::Image* image);
    void setStorageImage(gpu::ShaderBinding binding, gpu::Image* image);
    void setSampler(gpu::ShaderBinding binding, gpu::Sampler* sampler);
    void update();

  private:
    friend class TemporaryResourcePool;
    struct Binding {
        gpu::ShaderBinding binding;
        gpu::ResourceType type;
        const gpu::DeviceResource* resource = nullptr;
        uint64_t generation = 0;
        std::weak_ptr<gpu::DeviceResource> owner;
    };
    bool changed(gpu::ShaderBinding binding, gpu::ResourceType type, gpu::DeviceResource* resource, uint64_t generation);
    void releaseOwners();

    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::Buffer> _uniforms;
    std::shared_ptr<gpu::ResourceSet> _resources;
    std::shared_ptr<const gpu::detail::RecordingCompletion> _completion;
    uint32_t _set = 0;
    std::array<uint64_t,4> _identity{};
    uint64_t _uniformBytes = 0, _idleSince = 0;
    bool _dirty = false;
    std::vector<Binding> _tuple;
    std::vector<std::shared_ptr<gpu::DeviceResource>> _owners;
};

// Serial recording-thread pool. Limits bound completed retained cache storage;
// outstanding GPU work is never evicted to meet a budget. Call collect once per
// acquired-slot retirement. Callers retain returned images through every reader.
class BG2E_API TemporaryResourcePool {
  public:
    explicit TemporaryResourcePool(gpu::Device& device, TemporaryPoolLimits limits = {});

    void setEnabled(bool enabled);
    void collect();
    TemporaryPoolStatistics statistics() const;
    std::shared_ptr<gpu::Image> image(gpu::CommandBuffer& command, const gpu::ImageDescription& description);
    std::shared_ptr<TemporaryBindings> bindings(gpu::CommandBuffer& command,
        std::shared_ptr<gpu::PipelineLayout> layout, uint32_t set, uint64_t uniformBytes = 0, std::array<uint64_t,4> identity = {});

  private:
    struct ImageEntry {
        std::shared_ptr<gpu::Image> image;
        std::shared_ptr<const gpu::detail::RecordingCompletion> completion;
        uint64_t generation = 0, bytes = 0, idleSince = 0;
    };
    void trim();

    gpu::Device* _device;
    TemporaryPoolLimits _limits;
    bool _enabled = true;
    uint64_t _epoch = 1;
    std::vector<ImageEntry> _images;
    std::vector<std::shared_ptr<TemporaryBindings>> _bindings;
};
}
