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

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <functional>
#include <stack>
#include <unordered_map>
#include <vector>

namespace bg2e {
namespace manipulation {

class GizmoComponent;

// What kind of element a pick identifier refers to. Determines how the
// SelectionManager reacts to a hit.
enum class PickKind {
    Drawable,        // regular drawable submesh -> submesh selection
    Gizmo,           // light/environment/camera gizmo -> node selection
    TransformGizmo   // transform gizmo part -> transform interaction
};

class BG2E_API PickSelectionVisitor : public bg2e::scene::NodeVisitor {
public:
    struct SubmeshLookupData
    {
        bg2e::scene::Node * node;
        uint32_t submeshIndex;
        PickKind kind = PickKind::Drawable;
    };

    // depthPipeline: depth-tested pick pipeline (drawables and the transform
    // gizmo). noDepthPipeline: depth-disabled pick pipeline (type gizmos, drawn
    // on top of the scene). renderExtent: render-target extent, used to clear
    // the depth buffer before the transform gizmo.
    // includeTransformGizmo: when false the transform gizmo is not drawn in the
    // pick pass, so clicks pass through to whatever is behind it (used when
    // transform manipulation is disabled).
    void pick(
        bg2e::scene::Node* sceneRoot,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        VkPipeline depthPipeline,
        VkPipeline noDepthPipeline,
        const VkExtent2D & renderExtent,
        bool includeTransformGizmo
    );

    void visit(bg2e::scene::Node *) override;
    void didVisit(bg2e::scene::Node * node) override;

    SubmeshLookupData * findObject(uint32_t identifier);

protected:
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    VkPipeline _depthPipeline = VK_NULL_HANDLE;
    VkPipeline _noDepthPipeline = VK_NULL_HANDLE;
    VkPipeline _currentBoundPipeline = VK_NULL_HANDLE;
    VkExtent2D _renderExtent { 0, 0 };

    glm::mat4 _viewMatrix;
    glm::mat4 _projMatrix;

    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;

    std::unordered_map<uint32_t, SubmeshLookupData> _lookupNodes;

    // Type gizmos (light/environment/camera) are recorded during traversal and
    // drawn after it (depth-disabled) so they sit on top of the whole scene,
    // mirroring the visual gizmo renderer.
    struct PendingGizmo {
        bg2e::scene::Node * node;
        GizmoComponent * gizmo;
        glm::mat4 world;
        uint32_t identifier;
    };
    std::vector<PendingGizmo> _pendingGizmos;

    // The transform gizmo is drawn last over a cleared depth buffer.
    GizmoComponent * _pendingTransformGizmo = nullptr;
    glm::mat4 _pendingTransformWorld { 1.0f };
    bool _includeTransformGizmo = true;

    void bindPipeline(VkPipeline pipeline);
    void flushGizmos();
    void flushTransformGizmo();
};

}
}
