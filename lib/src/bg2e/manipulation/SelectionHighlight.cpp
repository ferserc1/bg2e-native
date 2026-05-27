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

#include <bg2e/manipulation/SelectionHighlight.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/all.hpp>

#include <iostream>

namespace bg2e::manipulation {

void SelectionHighlight::init(
    render::Engine * engine,
    VkSampleCountFlagBits sampleCount
) {
    _engine = engine;
    _sampleCount = sampleCount;
    createPipeline();
}

void SelectionHighlight::draw(
    bg2e::scene::Node * sceneRoot,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    VkCommandBuffer cmd
) {
    _viewProjectionMatrix = projMatrix * viewMatrix;
    _cmdBuffer = cmd;
    vkCmdBindPipeline(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipeline
    );
    sceneRoot->accept(this);
}

void SelectionHighlight::visit(bg2e::scene::Node * node)
{
    auto trx = node->getComponent<bg2e::scene::TransformComponent>();
    auto selectable = node->getComponent<bg2e::manipulation::SelectableComponent>();
    auto drawable = node->getComponent<bg2e::scene::DrawableComponent>();

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
                auto mat = drw->renderMaterial(i);
                auto albedo = mat->materialAttributes().albedo();
                FrameData frameData = {
                    .mvp = _viewProjectionMatrix * _currentTransform * drw->submeshTransform(i),
                    .color = glm::vec4{ albedo.r, albedo.g, albedo.b, _lineIntensity }
                };

                vkCmdPushConstants(
                    _cmdBuffer,
                    _pipelineLayout,
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

void SelectionHighlight::didVisit(bg2e::scene::Node * node)
{
    auto trx = node->getComponent<bg2e::scene::TransformComponent>();
    if (trx)
    {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

void SelectionHighlight::createPipeline()
{
    using namespace bg2e::render::vulkan;
    factory::GraphicsPipeline plFactory(_engine);

    plFactory.addShader("selection_highlight.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    plFactory.addShader("selection_highlight.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    render::vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addPushConstantRange(0, sizeof(FrameData), VK_SHADER_STAGE_VERTEX_BIT);
    auto pipelineLayout = layoutFactory.build("SelectionHighlight::PipelineLayout");
    _pipelineLayout = pipelineLayout;

    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_engine->swapchain().imageFormat());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    plFactory.disableDepthtest();
    plFactory.setPolygonMode(VK_POLYGON_MODE_LINE);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.enableBlendingAlphablend();
    plFactory.multisampling.rasterizationSamples = _sampleCount;

    auto pipeline = plFactory.build(_pipelineLayout, "SelectionHighlight::Pipeline");
    _pipeline = pipeline;
    _engine->cleanupManager().push([&, pipeline, pipelineLayout](VkDevice dev)
    {
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
    });
}

}
