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

#include <bg2e/manipulation/PickSelectionVisitor.hpp>

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>

namespace bg2e::manipulation {

void PickSelectionVisitor::pick(
    bg2e::scene::Node* sceneRoot,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkPipeline depthPipeline,
    VkPipeline noDepthPipeline,
    const VkExtent2D & renderExtent,
    bool includeTransformGizmo
) {
    _commandBuffer = cmd;
    _layout = layout;
    _depthPipeline = depthPipeline;
    _noDepthPipeline = noDepthPipeline;
    _renderExtent = renderExtent;
    _includeTransformGizmo = includeTransformGizmo;
    _currentBoundPipeline = VK_NULL_HANDLE;
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    _lookupNodes.clear();
    _pendingGizmos.clear();
    _pendingTransformGizmo = nullptr;

    sceneRoot->accept(this);

    // Order matches the visual renderer: scene drawables first (done during the
    // traversal above), then type gizmos on top, then the transform gizmo last
    // over a cleared depth buffer.
    flushGizmos();
    flushTransformGizmo();
}

void PickSelectionVisitor::bindPipeline(VkPipeline pipeline)
{
    if (_currentBoundPipeline != pipeline)
    {
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        _currentBoundPipeline = pipeline;
    }
}

void PickSelectionVisitor::visit(bg2e::scene::Node * node)
{
    auto drawable = node->getComponent<bg2e::scene::DrawableComponent>();
    auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
    auto selectableComponent = node->getComponent<bg2e::manipulation::SelectableComponent>();
    auto gizmo = node->getComponent<bg2e::manipulation::GizmoComponent>();

    if (transformComponent)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * transformComponent->matrix();
    }

    if (_commandBuffer == VK_NULL_HANDLE || _layout == VK_NULL_HANDLE)
    {
        return;
    }

    // Regular drawable submeshes (depth tested).
    if (selectableComponent && drawable)
    {
        auto drw = dynamic_cast<bg2e::scene::Drawable*>(drawable->drawableBase().get());
        auto mesh = drw->renderMesh();
        bindPipeline(_depthPipeline);
        for (uint32_t submesh = 0; submesh < mesh->submeshCount(); ++submesh)
        {
            auto submeshTransform = drw->submeshTransform(submesh);
            SelectionManager::PushConstantData pushConstants;
            uint32_t submeshIdentifier = selectableComponent->identifier(submesh);
            pushConstants.identifier = submeshIdentifier;
            _lookupNodes[submeshIdentifier] = {
                .node = node,
                .submeshIndex = submesh,
                .kind = PickKind::Drawable
            };
            pushConstants.mvp = _projMatrix * _viewMatrix * _currentTransform * submeshTransform;
            vkCmdPushConstants(
                _commandBuffer,
                _layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(SelectionManager::PushConstantData),
                &pushConstants
            );

            mesh->drawSubmesh(_commandBuffer, submesh);
        }
    }

    // Type gizmo (light/environment/camera): pickable only if the node owns a
    // SelectableComponent. Recorded here and drawn after the traversal.
    if (gizmo && selectableComponent &&
        gizmo->currentGizmoType() != GizmoType::None &&
        GizmoComponent::isGizmoVisible(gizmo->currentGizmoType()))
    {
        _pendingGizmos.push_back({
            .node = node,
            .gizmo = gizmo,
            .world = _currentTransform,
            .identifier = selectableComponent->gizmoIdentifier()
        });
    }

    // Transform gizmo: pickable whenever it is visible, no SelectableComponent
    // required. Deferred and drawn last over a cleared depth buffer.
    if (_includeTransformGizmo && gizmo && gizmo->transformVisible() && transformComponent &&
        GizmoComponent::isGizmoVisible(GizmoType::Transform))
    {
        _pendingTransformGizmo = gizmo;
        _pendingTransformWorld = _currentTransform;
    }
}

void PickSelectionVisitor::didVisit(bg2e::scene::Node * node)
{
    auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
    if (transformComponent)
    {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

void PickSelectionVisitor::flushGizmos()
{
    if (_pendingGizmos.empty())
    {
        return;
    }

    // Depth-disabled: type gizmos always pick on top of the scene.
    bindPipeline(_noDepthPipeline);
    for (const auto & pending : _pendingGizmos)
    {
        auto drw = pending.gizmo->drawable();
        if (!drw)
        {
            continue;
        }
        auto mesh = drw->renderMesh();
        auto gizmoTransform = pending.gizmo->renderTransform(pending.world, _viewMatrix, _projMatrix);

        // All submeshes of the gizmo share the node identifier: a hit anywhere
        // selects the node.
        _lookupNodes[pending.identifier] = {
            .node = pending.node,
            .submeshIndex = 0,
            .kind = PickKind::Gizmo
        };

        for (uint32_t i = 0; i < drw->submeshesCount(); ++i)
        {
            SelectionManager::PushConstantData pushConstants;
            pushConstants.identifier = pending.identifier;
            pushConstants.mvp = _projMatrix * _viewMatrix * gizmoTransform * drw->submeshTransform(i);
            vkCmdPushConstants(
                _commandBuffer,
                _layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(SelectionManager::PushConstantData),
                &pushConstants
            );
            mesh->drawSubmesh(_commandBuffer, i);
        }
    }
}

void PickSelectionVisitor::flushTransformGizmo()
{
    if (!_pendingTransformGizmo)
    {
        return;
    }

    auto drw = _pendingTransformGizmo->transformDrawable();
    if (!drw)
    {
        return;
    }

    auto mesh = drw->renderMesh();
    auto gizmoTransform = _pendingTransformGizmo->renderTransform(
        _pendingTransformWorld, _viewMatrix, _projMatrix, GizmoType::Transform
    );

    // Clear the depth buffer so the transform gizmo is pickable on top of the
    // scene while still self-occluding via the depth-tested pipeline.
    VkClearAttachment depthClear {};
    depthClear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthClear.clearValue.depthStencil = { 1.0f, 0 };
    VkClearRect clearRect {};
    clearRect.rect.offset = { 0, 0 };
    clearRect.rect.extent = _renderExtent;
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;
    vkCmdClearAttachments(_commandBuffer, 1, &depthClear, 1, &clearRect);

    bindPipeline(_depthPipeline);
    for (uint32_t i = 0; i < drw->submeshesCount(); ++i)
    {
        if (!GizmoComponent::isTransformSubmeshVisible(drw->submeshName(i), drw->submeshGroupName(i)))
        {
            continue;
        }

        uint32_t identifier = _pendingTransformGizmo->transformSubmeshIdentifier(i);
        _lookupNodes[identifier] = {
            .node = _pendingTransformGizmo->ownerNode(),
            .submeshIndex = i,
            .kind = PickKind::TransformGizmo
        };

        SelectionManager::PushConstantData pushConstants;
        pushConstants.identifier = identifier;
        pushConstants.mvp = _projMatrix * _viewMatrix * gizmoTransform * drw->submeshTransform(i);
        vkCmdPushConstants(
            _commandBuffer,
            _layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(SelectionManager::PushConstantData),
            &pushConstants
        );
        mesh->drawSubmesh(_commandBuffer, i);
    }
}

PickSelectionVisitor::SubmeshLookupData * PickSelectionVisitor::findObject(uint32_t identifier)
{
    if (_lookupNodes.find(identifier) != _lookupNodes.end())
    {
        return &_lookupNodes[identifier];
    }
    return nullptr;
}

}
