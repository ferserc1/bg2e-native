//
//  ResizeViewportVisitor.cpp
#include <bg2e/scene/ResizeViewportVisitor.hpp>

#include <bg2e/scene/Node.hpp>

namespace bg2e::scene {

void ResizeViewportVisitor::resizeViewport(Node * sceneRoot, const math::Viewport& vp)
{
    _vp = vp;
    sceneRoot->accept(this);
}

void ResizeViewportVisitor::resizeViewport(Node *sceneRoot, const VkExtent2D &viewportExtent)
{
    _vp = math::Viewport(viewportExtent.width, viewportExtent.height);
    sceneRoot->accept(this);
}
    
void ResizeViewportVisitor::visit(Node * node)
{
    for (auto comp : node->components())
    {
        comp.second->resizeViewport(_vp);
    }
}

}
