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

#include <bg2e/scene/DrawVisitor.hpp>
#include <bg2e/scene/TransformComponent.hpp>
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
    auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
    
    if (transformComponent)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * transformComponent->matrix();
    }
    
    if (drawable && _drawFunction && _commandBuffer != VK_NULL_HANDLE && _pipelineLayout != VK_NULL_HANDLE)
    {
        drawable->draw(_currentTransform, _commandBuffer, _pipelineLayout, _drawFunction);
    }
}

void DrawVisitor::didVisit(bg2e::scene::Node * node)
{
    auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
    if (transformComponent)
    {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

}
