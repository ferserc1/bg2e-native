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
