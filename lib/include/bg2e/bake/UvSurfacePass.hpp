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
#include <bg2e/bake/UvSurfaceMesh.hpp>
#include <bg2e/draw/Drawable.hpp>
#include <bg2e/draw/pipeline/RTAmbientOcclusion.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <array>
#include <filesystem>

namespace bg2e::bake {
struct UvSurfaceConfiguration {
    std::filesystem::path shaderRoot;
    std::string shaderPrefix = "bake/uv_surface", shaderInterface = "bake.uvSurface.v1";
    uint32_t maxInvocations = 64; // Per completed slot/serial, independent targets.
};
// Treat as immutable from preparation until all submitted users complete.
struct UvSurfaceGeometry {
    std::shared_ptr<draw::Drawable> target;
    std::shared_ptr<draw::Drawable::DrawMesh> drawMesh;
    UvSurfaceMesh packed;
    std::shared_ptr<gpu::Buffer> vertices;
};
struct UvSurfaceInput {
    gpu::CommandBuffer& command;
    uint32_t slot;
    uint64_t serial, sourceGeneration, targetGeneration;
    std::shared_ptr<const UvSurfaceGeometry> geometry;
    glm::mat4 nodeWorld{1}; // Excludes Drawable and per-submesh transforms.
    gpu::Size2D extent;
};
struct BG2E_API UvSurfaceResult {
    // position RGBA32F, signed normal RGBA16F, submesh R32_UINT, chart R32_UINT,
    // coverage R8_UNORM. Cleared identity is UINT_MAX, chart zero means invalid.
    std::array<std::shared_ptr<gpu::Image>,5> images;
    std::shared_ptr<const UvSurfaceGeometry> geometry;
    uint64_t sourceGeneration = 0, targetGeneration = 0;

    draw::pipeline::SurfaceInputs surfaceInputs() const; // Borrowed through this result's owners.
};
struct UvSurfaceDiagnostic {
    gpu::Size2D extent;
    std::vector<glm::vec4> positions, normals;
    std::vector<uint32_t> submeshes, charts;
    std::vector<uint8_t> coverage;
};
// Device-only, depthless UV2 raster plus coverage adapter. Caller owns submission,
// source generations and completed-slot discipline. UV v=0 is the top image row.
class BG2E_API UvSurfacePass {
  public:
    explicit UvSurfacePass(UvSurfaceConfiguration configuration = {});
    virtual ~UvSurfacePass() = default;
    UvSurfacePass(const UvSurfacePass&) = delete;
    UvSurfacePass& operator=(const UvSurfacePass&) = delete;

    virtual void initialize(gpu::Device& device, gpu::BackendType backend, uint32_t slots);
    // Stopped producer boundary: validates actual uploaded mesh and builds immutable packed buffer.
    virtual std::shared_ptr<const UvSurfaceGeometry> prepare(std::shared_ptr<draw::Drawable> target);
    virtual void beginFrame(uint32_t slot, uint64_t serial); // Retire this completed slot once.
    virtual UvSurfaceResult record(const UvSurfaceInput& input); // Fresh output/descriptors per target.
    virtual void cleanup(); // Drains before releasing owned GPU resources.

    // Completed submitted color readback only; never submit a partially recorded result.
    static UvSurfaceDiagnostic readDiagnostic(const UvSurfaceResult& result);

  protected:
    struct Invocation {
        UvSurfaceResult result;
        std::vector<std::shared_ptr<gpu::Buffer>> uniforms;
        std::vector<std::shared_ptr<gpu::ResourceSet>> sets;
    };
    struct Slot {
        uint64_t serial = 0;
        bool prepared = false;
        std::vector<Invocation> invocations;
    };
    UvSurfaceConfiguration _configuration;
    gpu::Device* _device = nullptr;
    std::vector<Slot> _slots;
    std::shared_ptr<gpu::PipelineLayout> _layout, _coverageLayout;
    std::shared_ptr<gpu::GraphicsPipeline> _pipeline;
    std::shared_ptr<gpu::ComputePipeline> _coveragePipeline;
    std::shared_ptr<gpu::ShaderModule> _vertex, _fragment, _coverageShader;
    std::shared_ptr<gpu::Sampler> _nearest;
};
}
