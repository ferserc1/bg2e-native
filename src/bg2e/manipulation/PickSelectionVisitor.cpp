//
//  PickSelectionVisitor.cpp

#include <bg2e/manipulation/PickSelectionVisitor.hpp>

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>

namespace bg2e::manipulation {

void PickSelectionVisitor::pick(
    bg2e::scene::Node* sceneRoot,
    const glm::mat4 & viewMatrix,
    const glm::mat4 & projMatrix,
    VkCommandBuffer cmd,
    VkPipelineLayout layout
) {
    _commandBuffer = cmd;
    _layout = layout;
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    sceneRoot->accept(this);
}

void PickSelectionVisitor::visit(bg2e::scene::Node * node)
{
    auto drawable = node->getComponent<bg2e::scene::DrawableComponent>();
    auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
    auto selectableComponent = node->getComponent<bg2e::manipulation::SelectableComponent>();
    
    if (transformComponent)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * transformComponent->matrix();
    }
    
    if (selectableComponent && drawable && _commandBuffer != VK_NULL_HANDLE && _layout != VK_NULL_HANDLE)
    {
        auto drw = dynamic_cast<bg2e::scene::Drawable*>(drawable->drawable().get());
        auto mesh = drw->renderMesh();
        for (auto submesh = 0; submesh < mesh->submeshCount(); ++submesh)
        {
            auto submeshTransform = drw->submeshTransform(submesh);
            SelectionManager::PushConstantData pushConstants;
            pushConstants.identifier = selectableComponent->identifier(submesh);
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

}
