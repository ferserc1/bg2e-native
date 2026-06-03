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

#include <bg2e/scene/FindCameraVisitor.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e::scene {

void FindCameraVisitor::findCameras(Node * node)
{
    cleanup();
    node->accept(this);
}

void FindCameraVisitor::visit(Node * node)
{
    auto camera = node->camera();
    if (camera)
    {
        _cameras.push_back(std::dynamic_pointer_cast<CameraComponent>(camera->shared_from_this()));
    }
}

void FindCameraVisitor::cleanup()
{
    _cameras.clear();
}

}
