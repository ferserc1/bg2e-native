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

#include <bg2e/common.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/pipeline/OverlayPass.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/math/all.hpp>


namespace bg2e {
namespace manipulation {

class GizmoComponent;

class BG2E_API GizmoAndSelectionRenderer : public bg2e::scene::NodeVisitor, public draw::pipeline::OverlayPass {
  public:
    explicit GizmoAndSelectionRenderer(std::shared_ptr<draw::pipeline::MeshOverlayPass> graphics = {});

    // Optional application-owned tool, independent of Renderer attachment.
    // Direct draw callers also initialize the OverlayPass lifecycle before use.
    void init(draw::Engine* engine);

    void initialize(draw::Engine& engine, const draw::pipeline::OverlayConfiguration& configuration) override;
    void attachScene(std::shared_ptr<scene::Node> root) override;
    void resize(const draw::pipeline::OverlayConfiguration& configuration) override;
    bool prepareFrame() override;
    void update(const draw::pipeline::OverlayContext& context) override;
    void record(const draw::pipeline::OverlayContext& context) override;
    void cleanup() override;

    std::shared_ptr<draw::pipeline::MeshOverlayPass> graphics() const;

    // renderExtent must be the extent of the actual render target (color/depth
    // image), not the swapchain extent: the deferred renderer rescales, so they
    // differ. It is used to clamp the transform gizmo depth clear to the render
    // area.

    void visit(bg2e::scene::Node* node) override;
    void didVisit(bg2e::scene::Node* node) override;

    inline void setLineIntensity(float i)
    {
        _lineIntensity = i;
    }
    inline float getLineIntensity() const
    {
        return _lineIntensity;
    }

  protected:
    draw::Engine* _drawEngine = nullptr;
    float _lineIntensity = 0.8f;

    struct EdgeMesh {
        std::weak_ptr<draw::Drawable::DrawMesh> source;
        std::shared_ptr<draw::Drawable::DrawMesh> lines;
    };
    struct OverlayFrame {
        draw::pipeline::OverlayDrawList draws;
        uint64_t serial = 0;
        bool prepared = false;
    };

    void collectDrawNodes(scene::Node* root);
    bool cacheLines(const std::shared_ptr<draw::Drawable>& drawable);
    std::shared_ptr<draw::Drawable::DrawMesh> lineMesh(const std::shared_ptr<draw::Drawable::DrawMesh>& source) const;

    std::shared_ptr<draw::pipeline::MeshOverlayPass> _graphics;
    bool _overlayInitialized = false;
    std::shared_ptr<scene::Node> _drawScene;
    std::vector<std::weak_ptr<scene::Node>> _drawNodes;
    std::vector<EdgeMesh> _edgeMeshes;
    std::vector<OverlayFrame> _overlayFrames;

};

}
}
