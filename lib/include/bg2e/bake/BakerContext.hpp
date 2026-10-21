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

#include <bg2e/bake/LightmapSettings.hpp>
#include <bg2e/gpu/Common.hpp>
#include <memory>
#include <filesystem>

namespace bg2e::gpu { class Device; class CommandBuffer; }
namespace bg2e::scene { class Node; }
namespace bg2e::draw::environment { struct EnvironmentResources; }
namespace bg2e::draw::pipeline {
class RTAmbientOcclusion;
class RTGlobalIllumination;
class DenoiseFilter;
class TracingSnapshot;
}

namespace bg2e::bake {
class UvTemporalAccumulator;
class LightmapCompositionPass;
class UvSurfacePass; // Device-backed UV surface primitive, see UvSurfacePass.hpp.

// Null AO/GI selects bake-owned public draw defaults in the concrete executor.
// Null accumulator/composition selects bake defaults; null denoise disables
// filtering. Injected filtering requires chart isolation.
// Injected objects are exclusive to one bake lifecycle; never share mutable slots
// with a display renderer. Stop producers for initialize/cleanup/reconfiguration.
struct BakePassConfiguration {
    std::shared_ptr<UvSurfacePass> surface; // Null selects the bake UV surface primitive.
    std::shared_ptr<draw::pipeline::RTAmbientOcclusion> ambientOcclusion;
    std::shared_ptr<draw::pipeline::RTGlobalIllumination> globalIllumination;
    std::shared_ptr<draw::pipeline::DenoiseFilter> denoise;
    std::shared_ptr<UvTemporalAccumulator> accumulator;
    std::shared_ptr<LightmapCompositionPass> composition;
};
struct BG2E_API BakerContextConfiguration {
    gpu::Device* device = nullptr; // Borrowed; outlives context, snapshots/results.
    gpu::BackendType backend = gpu::BackendType::Vulkan;
    uint32_t frameSlots = 1;
    std::filesystem::path shaderRoot; // Empty: resolve deployed shader root at initialization.
    std::shared_ptr<scene::Node> sceneRoot;
    std::shared_ptr<const draw::environment::EnvironmentResources> environment;
    BakePassConfiguration passes;

    void validate() const; // Structural only. Required native profiles checked at prepare.
};
struct BakeFrame {
    gpu::CommandBuffer& command; // Borrowed, outside a scope, no submission by baker.
    uint32_t slot;
    uint64_t serial, sourceGeneration; // Completed slot and captured scene generation.
};
// Base contract; IntegratedBakerContext provides draw execution. No viewport renderer
// is required. Geometry/light/material/environment changes invalidate sources.
class BG2E_API BakerContext {
  public:
    virtual ~BakerContext() = default;
    BakerContext(const BakerContext&) = delete;
    BakerContext& operator=(const BakerContext&) = delete;

    virtual const BakerContextConfiguration& configuration() const = 0;
    virtual uint64_t sourceGeneration() const = 0;

    virtual void invalidateSources() = 0;
    virtual void cleanup() = 0; // Drain submitted work before releasing borrowed sources.

  protected:
    BakerContext() = default;
};

// Public integrated/standalone execution and batch orchestration.
class IntegratedBakerContext;
class StandaloneBakerContext;
class StandaloneBakeSceneAssembler;
class StandaloneBakeBatch;
class UvMapPreviewRenderer;
}
