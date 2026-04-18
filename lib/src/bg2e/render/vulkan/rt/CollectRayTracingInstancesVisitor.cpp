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

#include <bg2e/render/vulkan/rt/CollectRayTracingInstancesVisitor.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

void CollectRayTracingInstancesVisitor::visit(scene::Node * node)
{
    auto transformComponent = node->getComponent<scene::TransformComponent>();

    if (transformComponent)
    {
        _transformStack.push(_currentTransform);
        _currentTransform = _currentTransform * transformComponent->matrix();
    }

    // TODO: implement acceleration structure update
}

void CollectRayTracingInstancesVisitor::didVisit(scene::Node * node)
{
    auto transformComponent = node->getComponent<scene::TransformComponent>();

    if (transformComponent)
    {
        _currentTransform = _transformStack.top();
        _transformStack.pop();
    }
}

}
}
}
}
