//
//  Component.hpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/projections.hpp>

#include <string>

#define BG2E_SCENE_COMP_CLASS_NAME(c) typeid(*c).name()

namespace bg2e {
namespace scene {

class Node;
class Scene;

class BG2E_API Component {
    friend class Node;
public:
    
    virtual ~Component() = default;
    
    inline Node * ownerNode() const { return _owner; }
    Scene * scene();

    virtual void resizeViewport(const math::Viewport&) {}
    
    // Use this function to update animation data.
    virtual void animate(float delta) {}
    
    // Use this function to update matrixes generated from
    // the animation data.
    virtual void update(float delta) {}

protected:

    // Weak ptr to avoid circular references
    Node * _owner = nullptr;
    
    virtual void removedFromNode(Node * prevOwner) {}
    virtual void addedToNode(Node * newOwner) {}
};

extern BG2E_API std::string componentName(Component * comp);


}
}
