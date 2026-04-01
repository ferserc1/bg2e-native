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
    _lookupNodes.clear();
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
        auto drw = dynamic_cast<bg2e::scene::Drawable*>(drawable->drawableBase().get());
        auto mesh = drw->renderMesh();
        for (uint32_t submesh = 0; submesh < mesh->submeshCount(); ++submesh)
        {
            auto submeshTransform = drw->submeshTransform(submesh);
            SelectionManager::PushConstantData pushConstants;
            uint32_t submeshIdentifier = selectableComponent->identifier(submesh);
            pushConstants.identifier = submeshIdentifier;
            _lookupNodes[submeshIdentifier] = {
                .node = node,
                .submeshIndex = submesh
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

PickSelectionVisitor::SubmeshLookupData * PickSelectionVisitor::findObject(uint32_t identifier)
{
    if (_lookupNodes.find(identifier) != _lookupNodes.end())
    {
        return &_lookupNodes[identifier];
    }
    return nullptr;
}

}
