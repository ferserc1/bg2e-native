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

#include <bg2e/manipulation/GizmoAndSelectionRenderer.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/all.hpp>
#include <bg2e/scene/LightComponent.hpp>

namespace bg2e::manipulation {

void GizmoAndSelectionRenderer::init(
    render::Engine * engine,
    VkSampleCountFlagBits sampleCount
) {
    _engine = engine;
    _sampleCount = sampleCount;
    createSelectionPipeline();
    createGizmoPipeline();
    createOpaqueGizmoPipeline();
}

void GizmoAndSelectionRenderer::draw(
    bg2e::scene::Node * sceneRoot,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    VkCommandBuffer cmd,
    const VkExtent2D & renderExtent
) {
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    _viewProjectionMatrix = projMatrix * viewMatrix;
    _cmdBuffer = cmd;
    _renderExtent = renderExtent;
    _currentBoundPipeline = VK_NULL_HANDLE;
    _pendingTransformGizmo = nullptr;
    sceneRoot->accept(this);

    // Draw the transform gizmo last, so it sits on top of every other gizmo.
    flushTransformGizmo();
}

void GizmoAndSelectionRenderer::visit(bg2e::scene::Node * node)
{
    auto trx = node->getComponent<bg2e::scene::TransformComponent>();
    auto selectable = node->getComponent<bg2e::manipulation::SelectableComponent>();
    auto drawable = node->getComponent<bg2e::scene::DrawableComponent>();
    auto gizmo = node->getComponent<bg2e::manipulation::GizmoComponent>();

    if (trx)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * trx->matrix();
    }

    if (selectable && drawable)
    {
        auto drw = drawable->drawable();
        auto mesh = drw->renderMesh();
        auto submeshes = drawable->drawable()->submeshesCount();
        for (uint32_t i = 0; i < submeshes; ++i)
        {
            if (selectable->isSelected(i))
            {
                if (_currentBoundPipeline != _selectionPipeline)
                {
                    vkCmdBindPipeline(_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _selectionPipeline);
                    _currentBoundPipeline = _selectionPipeline;
                }

                auto mat = drw->renderMaterial(i);
                auto albedo = mat->materialAttributes().albedo();
                FrameData frameData = {
                    .mvp = _viewProjectionMatrix * _currentTransform * drw->submeshTransform(i),
                    .color = glm::vec4{ albedo.r, albedo.g, albedo.b, _lineIntensity }
                };

                vkCmdPushConstants(
                    _cmdBuffer,
                    _selectionPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(FrameData),
                    &frameData
                );

                mesh->drawSubmesh(_cmdBuffer, i);
            }
        }
    }

    // Transform gizmo facet. Independent from the type gizmo below: it is drawn
    // when this node is the current transform node, owns a TransformComponent
    // and the transform gizmo is globally visible. It is recorded here and drawn
    // last (see flushTransformGizmo) so it always sits on top of every other
    // gizmo and self-occludes via the opaque depth-tested pipeline.
    if (gizmo && gizmo->transformVisible() && trx &&
        GizmoComponent::isGizmoVisible(GizmoType::Transform))
    {
        _pendingTransformGizmo = gizmo;
        _pendingTransformWorld = _currentTransform;
    }

    if (gizmo && gizmo->currentGizmoType() != GizmoType::None)
    {
        auto type = gizmo->currentGizmoType();
        if (!GizmoComponent::isGizmoVisible(type)) return;

        auto drw = gizmo->drawable();
        if (drw)
        {
            auto mesh = drw->renderMesh();
            auto submeshes = drw->submeshesCount();
            auto gizmoTransform = gizmo->renderTransform(_currentTransform, _viewMatrix, _projMatrix);
            auto gizmoColor = gizmo->gizmoColor();
            float opacity = GizmoComponent::gizmoOpacity(type);
            for (uint32_t i = 0; i < submeshes; ++i)
            {
                if (_currentBoundPipeline != _gizmoPipeline)
                {
                    vkCmdBindPipeline(_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _gizmoPipeline);
                    _currentBoundPipeline = _gizmoPipeline;
                }

                FrameData frameData = {
                    .mvp = _viewProjectionMatrix * gizmoTransform * drw->submeshTransform(i),
                    .color = glm::vec4{ gizmoColor.r, gizmoColor.g, gizmoColor.b, opacity }
                };

                vkCmdPushConstants(
                    _cmdBuffer,
                    _gizmoPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(FrameData),
                    &frameData
                );

                mesh->drawSubmesh(_cmdBuffer, i);
            }
        }
    }
}

void GizmoAndSelectionRenderer::didVisit(bg2e::scene::Node * node)
{
    auto trx = node->getComponent<bg2e::scene::TransformComponent>();
    if (trx)
    {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

void GizmoAndSelectionRenderer::flushTransformGizmo()
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
    auto submeshes = drw->submeshesCount();
    auto gizmoTransform = _pendingTransformGizmo->renderTransform(
        _pendingTransformWorld, _viewMatrix, _projMatrix, GizmoType::Transform
    );
    float opacity = GizmoComponent::gizmoOpacity(GizmoType::Transform);

    // Clear the depth buffer so the transform gizmo always draws on top of the
    // scene while still self-occluding via the opaque pipeline. Safe because
    // nothing samples depth after the gizmo pass (the depth buffer is only used
    // as an attachment within the scene render).
    VkClearAttachment depthClear {};
    depthClear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthClear.clearValue.depthStencil = { 1.0f, 0 };
    VkClearRect clearRect {};
    clearRect.rect.offset = { 0, 0 };
    clearRect.rect.extent = _renderExtent;
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;
    vkCmdClearAttachments(_cmdBuffer, 1, &depthClear, 1, &clearRect);

    vkCmdBindPipeline(_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _opaqueGizmoPipeline);
    _currentBoundPipeline = _opaqueGizmoPipeline;

    for (uint32_t i = 0; i < submeshes; ++i)
    {
        if (!GizmoComponent::isTransformSubmeshVisible(drw->submeshName(i), drw->submeshGroupName(i)))
        {
            continue;
        }

        auto albedo = drw->renderMaterial(i)->materialAttributes().albedo();
        FrameData frameData = {
            .mvp = _viewProjectionMatrix * gizmoTransform * drw->submeshTransform(i),
            .color = glm::vec4{ albedo.r, albedo.g, albedo.b, opacity }
        };

        vkCmdPushConstants(
            _cmdBuffer,
            _gizmoPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(FrameData),
            &frameData
        );

        mesh->drawSubmesh(_cmdBuffer, i);
    }
}

void GizmoAndSelectionRenderer::createSelectionPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("selection_highlight.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("selection_highlight.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    render::vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addPushConstantRange(0, sizeof(FrameData), VK_SHADER_STAGE_VERTEX_BIT);
    auto pipelineLayout = layoutFactory.build("GizmoAndSelectionRenderer::SelectionPipelineLayout");
    _selectionPipelineLayout = pipelineLayout;

    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_engine->swapchain().imageFormat());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    plFactory.disableDepthtest();
    plFactory.setPolygonMode(VK_POLYGON_MODE_LINE);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.enableBlendingAlphablend();
    plFactory.multisampling.rasterizationSamples = _sampleCount;

    auto pipeline = plFactory.build(_selectionPipelineLayout, "GizmoAndSelectionRenderer::SelectionPipeline");
    _selectionPipeline = pipeline;
    _engine->cleanupManager().push([&, pipeline, pipelineLayout](VkDevice dev)
    {
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
    });
}

void GizmoAndSelectionRenderer::createGizmoPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("gizmo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("gizmo.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    render::vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addPushConstantRange(0, sizeof(FrameData), VK_SHADER_STAGE_VERTEX_BIT);
    auto pipelineLayout = layoutFactory.build("GizmoAndSelectionRenderer::GizmoPipelineLayout");
    _gizmoPipelineLayout = pipelineLayout;

    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_engine->swapchain().imageFormat());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    plFactory.disableDepthtest();
    plFactory.setPolygonMode(VK_POLYGON_MODE_FILL);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.enableBlendingAlphablend();
    plFactory.multisampling.rasterizationSamples = _sampleCount;

    auto pipeline = plFactory.build(_gizmoPipelineLayout, "GizmoAndSelectionRenderer::GizmoPipeline");
    _gizmoPipeline = pipeline;
    _engine->cleanupManager().push([&, pipeline, pipelineLayout](VkDevice dev)
    {
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
    });
}

void GizmoAndSelectionRenderer::createOpaqueGizmoPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("gizmo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("gizmo.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // Reuse the gizmo pipeline layout: same FrameData push constant range.
    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_engine->swapchain().imageFormat());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    // Opaque, depth-tested and depth-writing so the transform gizmo handles
    // occlude each other correctly. No blending (unlike the translucent gizmo
    // pipeline, which disables depth test).
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    plFactory.setPolygonMode(VK_POLYGON_MODE_FILL);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.multisampling.rasterizationSamples = _sampleCount;

    auto pipeline = plFactory.build(_gizmoPipelineLayout, "GizmoAndSelectionRenderer::OpaqueGizmoPipeline");
    _opaqueGizmoPipeline = pipeline;
    _engine->cleanupManager().push([&, pipeline](VkDevice dev)
    {
        vkDestroyPipeline(dev, pipeline, nullptr);
    });
}

}
