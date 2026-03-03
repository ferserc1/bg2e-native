
#include <bg2e/manipulation/SelectionHighlight.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/render/all.hpp>

#include <iostream>

namespace bg2e::manipulation {

void SelectionHighlight::init(
    render::Engine * engine
) {
    _engine = engine;
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

    VkPushConstantRange pushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(FrameData);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    auto layoutInfo = Info::pipelineLayoutInfo();
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    layoutInfo.pushConstantRangeCount = 1;

    VK_ASSERT(vkCreatePipelineLayout(
        _engine->device().handle(),
        &layoutInfo,
        nullptr,
        &_pipelineLayout
    ));

    plFactory.setInputState<render::vulkan::geo::Mesh>();
    plFactory.setColorAttachmentFormat(_engine->swapchain().imageFormat());
    plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
    plFactory.disableDepthtest();
    plFactory.setPolygonMode(VK_POLYGON_MODE_LINE);
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    plFactory.enableBlendingAlphablend();
    plFactory.multisampling.rasterizationSamples = _engine->swapchain().sampleCount();

    _pipeline = plFactory.build(_pipelineLayout);

    _engine->cleanupManager().push([&](VkDevice dev)
    {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
    });
}

}
