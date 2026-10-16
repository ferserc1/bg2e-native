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

#include <bg2e/draw/RenderQueue.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/ShaderModule.hpp>
#include <filesystem>

namespace bg2e::draw::pipeline {

struct OverlayConfiguration {
    gpu::Size2D extent;
    gpu::PixelFormat colorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
    gpu::PixelFormat depthFormat = gpu::PixelFormat::D32_SFLOAT;
    uint32_t frameSlots = 0;
};

// Borrowed only during update/record. The acquired slot has completed its previous
// submission. Camera matrices are original, unjittered; color is final output.
// Keep all submitted GPU resources alive until that slot completes. No scene
// mutations, immediate submissions, readback or GPU waits during these calls.
struct OverlayContext {
    const FrameContext& frame;
    scene::Node* sceneRoot;
    const RenderQueue& queue;
    const glm::mat4& view;
    const glm::mat4& projection;
};

// Optional public renderer extension. One instance per renderer lifecycle;
// initialize/attachScene/resize/prepareFrame/cleanup run at stopped producer
// boundaries. prepareFrame runs before acquisition and may realize resources.
// update/record run once per scene frame, after scene update / final output.
class BG2E_API OverlayPass {
  public:
    virtual ~OverlayPass() = default;

    virtual void initialize(Engine& engine, const OverlayConfiguration& configuration) = 0;
    virtual void attachScene(std::shared_ptr<scene::Node> root) = 0;
    virtual void resize(const OverlayConfiguration& configuration) = 0;
    virtual bool prepareFrame() = 0;
    virtual void update(const OverlayContext& context) = 0;
    virtual void record(const OverlayContext& context) = 0;
    virtual void cleanup() = 0;
};

struct OverlayDraw {
    std::shared_ptr<Drawable::DrawMesh> mesh;
    uint32_t submesh = 0;
    glm::mat4 world{1};
    glm::vec4 color{1};
};

struct OverlayDrawList {
    std::vector<OverlayDraw> occluders;
    std::vector<OverlayDraw> highlights;
    std::vector<OverlayDraw> typeGizmos;
    std::vector<OverlayDraw> transformGizmos;
};

// Settings are fixed for a graphics lifecycle. Reinitialize at a drained
// boundary to change topology/depth/blend/shaders. Portable lines are one pixel.
struct MeshOverlaySettings {
    std::filesystem::path shaderRoot;
    std::string highlightShader = "draw/selection_highlight/highlight";
    std::string gizmoShader = "draw/gizmo/gizmo";
    bool highlightLines = true;
    bool typeGizmoLines = true;
    bool highlightDepthTest = false;
    bool typeGizmoDepthTest = false;
    bool highlightBlend = true;
    bool typeGizmoBlend = true;
    bool transformBlend = false;
    float highlightDepthBias = 0.00001f; // Clip depth divided by w; avoids edge z-fighting.
};

// Public replaceable GPU component. No manipulation/selection dependencies.
// Depth-tested overlays rerasterize supplied occluders at output resolution
// without jitter; top overlays ignore that depth. Transform gizmos use separate
// fresh output depth and draw last, self-occluding. record retains draw owners
// per acquired slot, loads final color and leaves it ShaderReadOnly.
class BG2E_API MeshOverlayPass {
  public:
    explicit MeshOverlayPass(MeshOverlaySettings settings = {});
    virtual ~MeshOverlayPass() = default;

    virtual void initialize(Engine& engine, const OverlayConfiguration& configuration);
    virtual void resize(const OverlayConfiguration& configuration);
    virtual void record(const OverlayContext& context, const OverlayDrawList& draws);
    virtual void cleanup();

    const MeshOverlaySettings& settings() const;

  protected:
    struct Slot {
        std::shared_ptr<gpu::Image> sceneDepth, transformDepth;
        OverlayDrawList draws;
    };

    void buildDepth();
    void validateDraw(const OverlayDraw& draw) const;

    MeshOverlaySettings _settings;
    Engine* _engine = nullptr;
    OverlayConfiguration _configuration;
    std::shared_ptr<gpu::PipelineLayout> _layout;
    std::vector<std::shared_ptr<gpu::ShaderModule>> _shaders;
    std::shared_ptr<gpu::GraphicsPipeline> _depth, _highlight, _type, _transform;
    std::vector<Slot> _slots;
};

}
