//
//  FindNodeVisitor.cpp
#include <bg2e/scene/FindNodeVisitor.hpp>

namespace bg2e::scene {

bool FindNodeVisitor::belongsToScene(Node * findNode, Node * sceneRoot)
{
    _found = false;
    _findNode = findNode;
    sceneRoot->accept(this);
    return _found;
}

void FindNodeVisitor::visit(Node * node)
{
    if (node == _findNode)
    {
        _found = true;
    }
}

}
