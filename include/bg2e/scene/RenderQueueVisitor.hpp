//
//  RenderQueueVisitor.hpp
//  bg2e
//

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
    RenderQueueVisitor() {}
    
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
