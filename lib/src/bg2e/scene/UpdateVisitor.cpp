//
//  UpdateVisitor.cpp
#include <bg2e/scene/UpdateVisitor.hpp>

#include <bg2e/scene/Node.hpp>

namespace bg2e::scene {


void UpdateVisitor::update(Node * sceneRoot, float delta)
{
    _delta = delta;
    sceneRoot->accept(this);
}

void UpdateVisitor::visit(Node * node)
{
    // Animation
    for (auto comp : node->components())
    {
        comp.second->animate(_delta);
    }
    
    // Update
    for (auto comp : node->components())
    {
        comp.second->update(_delta);
    }
}

}
