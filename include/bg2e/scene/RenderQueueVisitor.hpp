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

template <typename RenderMeshT>
class BG2E_API RenderQueueVisitor : public bg2e::scene::NodeVisitor {
public:
    RenderQueueVisitor() {}
    
    void enqueue(
        bg2e::scene::Node* sceneRoot,
        bg2e::render::RenderQueue<RenderMeshT> * renderQueue
    ) {
        _renderQueue = renderQueue;
        _renderQueue->beginFrame();
        sceneRoot->accept(this);
    }
    
    void visit(bg2e::scene::Node * node)
    {
        auto drawableComponent = node->getComponent<bg2e::scene::DrawableComponent>();
        auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
        
        if (transformComponent)
        {
            _transformStack.push(_currentTransform);
            _currentTransform = _currentTransform * transformComponent->matrix();
        }
        
        if (drawableComponent)
        {
            Drawable * drawable = dynamic_cast<Drawable*>(drawableComponent->drawable().get());
            if (drawable)
            {
                for (auto i = 0; i < drawable->materials().size(); ++i)
                {
                    std::shared_ptr<render::MaterialBase> mat = drawable->renderMaterial(i);
                    auto submeshTransform = drawable->submeshTransform(i);
                    auto trx = _currentTransform * submeshTransform;
                    std::shared_ptr<render::vulkan::geo::Mesh> renderMesh = drawable->renderMesh();
                    _renderQueue->enqueue(renderMesh, i, mat, trx);
                }
            }
        }
    }
    
    void didVisit(bg2e::scene::Node * node)
    {
        auto transformComponent = node->getComponent<bg2e::scene::TransformComponent>();
        if (transformComponent)
        {
            _currentTransform = _transformStack.top();
            _transformStack.pop();
        }
    }

protected:
    bg2e::render::RenderQueue<RenderMeshT> * _renderQueue;
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;
};

}
}
