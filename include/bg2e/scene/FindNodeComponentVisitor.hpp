//
//  FindComponentTypeVisitor.hpp
#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/Node.hpp>
#include <vector>

namespace bg2e {
namespace scene {

template <typename ComponentT>
class FindNodeComponentVisitor : public NodeVisitor {
public:
    const std::vector<Node*>& find(Node* sceneRoot)
    {
        _result.clear();
        sceneRoot->accept(this);
        return _result;
    }
    
    void visit(Node * node)
    {
        if (node->getComponent<ComponentT>())
        {
            _result.push_back(node);
        }
    }
    
protected:
    std::vector<Node*> _result;
};

}
}
