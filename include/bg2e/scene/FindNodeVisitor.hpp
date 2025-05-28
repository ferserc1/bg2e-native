//
//  FindNodeVisitor.hpp

#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e {
namespace scene {

class BG2E_API FindNodeVisitor : public NodeVisitor {
public:
    bool belongsToScene(Node * findNode, Node * sceneRoot);
    
    void visit(Node *);
    
    // Cancel if node is found
    virtual bool cancel() { return _found; }
    
protected:
    bool _found = false;
    Node * _findNode = nullptr;
};

}
}
