//
//  DrawVisitor.cpp
#include <bg2e/scene/DrawVisitor.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e::scene {

void DrawVisitor::draw(
    bg2e::scene::Node* sceneRoot,
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    bg2e::scene::DrawableBase::DrawFunction cb
) {
    _commandBuffer = cmd;
    _pipelineLayout = layout;
    _drawFunction = cb;
    sceneRoot->accept(this);
}

void DrawVisitor::visit(bg2e::scene::Node * node)
{
    auto drawable = node->getComponent<bg2e::scene::DrawableComponent>();
    
    if (drawable && _drawFunction && _commandBuffer != VK_NULL_HANDLE && _pipelineLayout != VK_NULL_HANDLE)
    {
        // TODO: Apply node transformation matrix to the drawable transform
        auto nodeTransform = glm::mat4 { 1.0f };
        drawable->draw(nodeTransform, _commandBuffer, _pipelineLayout, _drawFunction);
    }
}

}
