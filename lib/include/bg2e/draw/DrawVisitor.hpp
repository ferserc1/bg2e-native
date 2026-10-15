#pragma once

#include <bg2e/draw/RenderQueueVisitor.hpp>

namespace bg2e::draw {
class BG2E_API DrawVisitor {
  public:
    using BindFunction = std::function<void(const RenderQueue::Item&)>;
    // Pipeline/rendering scope are caller-owned. Callback binds per-item data.
    void draw(const FrameContext& frame, const RenderQueue& queue, RenderQueueType type, const BindFunction& bind) const;
    // Convenience extraction/sorting; retained queue keeps resources alive after recording.
    void draw(scene::Node* root, const FrameContext& frame, RenderQueue& retainedQueue,
        RenderQueueType type, const glm::vec3& cameraPosition, const BindFunction& bind) const;
};
}
