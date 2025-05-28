//
//  NodeVisitor.hpp

#pragma once

#include <bg2e/common.hpp>

namespace bg2e {
namespace scene {

class Node;

class NodeVisitor {
public:
    virtual ~NodeVisitor() = default;
    
    virtual void visit(Node * node) {}
    virtual void didVisit(Node * node) {}
    
    inline void setIgnoreDisabled(bool i) { _ignoreDisabled = i; }
    inline bool ignoreDisabled() const { return _ignoreDisabled; }
    
    virtual bool cancel() { return false; }
    
protected:
    bool _ignoreDisabled = false;
};

}
}
