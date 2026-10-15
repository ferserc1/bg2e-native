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

#include <bg2e/draw/DrawVisitor.hpp>
#include <stdexcept>

namespace bg2e::draw {
void DrawVisitor::draw(const FrameContext& frame, const RenderQueue& queue, RenderQueueType type, const BindFunction& bind) const
{
    if (queue.engine() != &frame.engine || !bind)
    {
        throw std::invalid_argument("Draw queue and binding callback must match the current frame");
    }
    queue.validate(frame.engine);
    for (const auto& item : queue.queue(type))
    {
        frame.commandBuffer.bindVertexBuffer(0, item.drawMesh->vertexBuffer(), 0);
        frame.commandBuffer.bindIndexBuffer(item.drawMesh->indexBuffer(), 0);
        bind(item);
        item.drawMesh->drawSubmesh(&frame.commandBuffer, item.submeshIndex);
    }
}
void DrawVisitor::draw(scene::Node* root, const FrameContext& frame, RenderQueue& retainedQueue,
    RenderQueueType type, const glm::vec3& cameraPosition, const BindFunction& bind) const
{
    RenderQueueVisitor visitor;
    visitor.enqueue(root, frame.engine, retainedQueue);
    retainedQueue.sort(cameraPosition);
    draw(frame, retainedQueue, type, bind);
}
}
