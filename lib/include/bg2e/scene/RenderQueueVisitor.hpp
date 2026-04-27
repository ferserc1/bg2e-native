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

#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/render/RenderQueue.hpp>

#include <stack>

namespace bg2e {
namespace scene {

template <typename DrawableT>
class BG2E_API RenderQueueVisitor : public bg2e::scene::NodeVisitor {
public:
    RenderQueueVisitor();
    
    void enqueue(bg2e::scene::Node* sceneRoot, bg2e::render::RenderQueue<DrawableT>* renderQueue);
    void visit(bg2e::scene::Node* node);
    void didVisit(bg2e::scene::Node* node);

protected:
    bg2e::render::RenderQueue<DrawableT> * _renderQueue;
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;
};

}
}
