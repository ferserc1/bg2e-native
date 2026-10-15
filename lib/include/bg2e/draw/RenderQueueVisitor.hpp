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

#include <bg2e/draw/RenderQueue.hpp>
#include <bg2e/scene/NodeVisitor.hpp>

namespace bg2e::draw {
class BG2E_API RenderQueueVisitor : public scene::NodeVisitor {
  public:
    RenderQueueVisitor();
    // Publishes a complete snapshot or leaves the previous queue intact.
    void enqueue(scene::Node* root, Engine& engine, RenderQueue& queue);
    void visit(scene::Node* node) override;
    void didVisit(scene::Node* node) override;
  private:
    RenderQueue* _queue = nullptr;
    glm::mat4 _world{1};
    std::vector<glm::mat4> _stack;
};
}
