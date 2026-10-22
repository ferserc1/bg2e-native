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

#include <bg2e/draw/pipeline/GBuffer.hpp>
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <span>

namespace bg2e::draw::pipeline {
// objectId selects this extracted queue's bindings; it is not persistent identity.
// Canonical tokens come from renderer correspondence. Invalid matches still
// draw/write depth, but emit zero so an occluder cannot expose a token behind it.
struct TemporalSurfaceInstance {
    uint32_t objectId = 0, token = 0;
    bool correspondenceValid = false;
    // Optional recorded affine model; absence means the current model (static caller).
    glm::mat4 previousWorld{1};
    bool hasPreviousWorld = false;
};
struct TemporalSurfaceResult {
    std::shared_ptr<gpu::Image> token, depth;
    std::shared_ptr<gpu::Image> motion, previousPosition, previousNormal;
    bool legacyCompatible = true; // P1 consumer cannot transport moving receivers.
};

struct TemporalSurfaceCamera {
    glm::mat4 currentViewProjection{1}, previousViewProjection{1}; // Unjittered.
};

// Separate full-extent receiver replay for the exact built-in GBuffer contract.
// initialize/cleanup require stopped users. Inputs/bindings belong to completed-slot
// rings and remain alive through GPU completion. Returned images are immutable after
// recording; retain every consumed image through its readers, and release before Device.
class BG2E_API TemporalSurfacePass {
  public:
    explicit TemporalSurfacePass(std::filesystem::path shaderRoot = {}, TemporaryPoolLimits limits = {});
    TemporalSurfacePass(const TemporalSurfacePass&) = delete;
    TemporalSurfacePass& operator=(const TemporalSurfacePass&) = delete;

    void initialize(Engine& engine, const RenderLayer& receiver, uint32_t frameSlots = 0);
    void beginFrame(uint32_t slot, uint64_t serial);
    void cleanup();

    // Null raster.item replays the opaque queue with freshly cleared private depth.
    // A single transparent item requires preserved opaque depth (same extent/format,
    // sampled + transfer-source), copied to a new private depth before rendering.
    // raster.region is deliberately ignored: metadata clears/draws the full extent.
    // Missing instance entries emit zero. Supplied IDs must be unique/current.
    // Unsupported initialization returns empty metadata without recording GPU work.
    TemporalSurfaceResult record(const FrameContext& frame, const RasterFrame& raster,
        gpu::Size2D extent, std::span<const TemporalSurfaceInstance> instances,
        gpu::Image* opaqueDepth = nullptr, uint64_t view = 0, uint64_t layer = 0,
        const TemporalSurfaceCamera& camera = {});

    bool available() const;
    const std::string& status() const;

  private:
    struct Invocation {
        TemporalSurfaceResult result;
        std::vector<RenderQueue::Item> items;
        std::vector<std::shared_ptr<TemporaryBindings>> bindings;
        uint64_t view = 0, layer = 0;
    };
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false;
        std::vector<std::shared_ptr<Invocation>> invocations;
    };
    std::filesystem::path _shaderRoot;
    TemporaryPoolLimits _limits;
    Engine* _engine = nullptr;
    gpu::PixelFormat _depthFormat = gpu::PixelFormat::Undefined;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::ShaderModule> _vertex, _fragment;
    std::shared_ptr<TemporaryResourcePool> _pool;
    std::vector<Slot> _slots;
    std::string _status = "Not initialized";
};
}
