//
//  Component.hpp
#pragma once

#include <bg2e/common.hpp>

#include <string>

namespace bg2e {
namespace scene {

class Node;

class Component {
    friend class Node;
public:
    
    virtual ~Component() = default;
    
    inline Node * ownerNode() { return _owner; }

    virtual void update(float delta) {}

protected:

    // Weak ptr to avoid circular references
    Node * _owner = nullptr;
    
    virtual void removedFromNode(Node * prevOwner) {}
    virtual void addedToNode(Node * newOwner) {}
};

extern BG2E_API size_t componentHash(Component * comp);


}
}
