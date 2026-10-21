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
#include <bg2e/scene/Node.hpp>
#include <stdexcept>
#include <utility>

namespace bg2e::manipulation {

GizmoAndSelectionRenderer::GizmoAndSelectionRenderer(std::shared_ptr<draw::pipeline::MeshOverlayPass> graphics)
    : _graphics(std::move(graphics))
{
}

void GizmoAndSelectionRenderer::init(draw::Engine* engine)
{
    if (!engine || _drawEngine)
    {
        throw std::logic_error("Overlay requires an engine and cannot be initialized twice");
    }
    _drawEngine = engine;
}

void GizmoAndSelectionRenderer::visit(scene::Node* node)
{
    _drawNodes.push_back(node->weak_from_this());
}

void GizmoAndSelectionRenderer::didVisit(scene::Node*)
{
}

}
