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
