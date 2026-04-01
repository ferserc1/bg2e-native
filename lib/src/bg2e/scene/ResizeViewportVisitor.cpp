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

#include <bg2e/scene/ResizeViewportVisitor.hpp>

#include <bg2e/scene/Node.hpp>

namespace bg2e::scene {

void ResizeViewportVisitor::resizeViewport(Node * sceneRoot, const math::Viewport& vp)
{
    _vp = vp;
    sceneRoot->accept(this);
}

void ResizeViewportVisitor::resizeViewport(Node *sceneRoot, const VkExtent2D &viewportExtent)
{
    _vp = math::Viewport(viewportExtent.width, viewportExtent.height);
    sceneRoot->accept(this);
}
    
void ResizeViewportVisitor::visit(Node * node)
{
    for (auto comp : node->components())
    {
        comp.second->resizeViewport(_vp);
    }
}

}
