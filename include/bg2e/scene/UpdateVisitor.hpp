//
//  UpdateVisitor.hpp
#pragma once

#include <bg2e/scene/NodeVisitor.hpp>

namespace bg2e {
namespace scene {

class BG2E_API UpdateVisitor : public NodeVisitor {
public:
    void update(Node * sceneRoot, float delta);
    
    void visit(Node *) override;

protected:
    float _delta = 0.0f;
};

}
}
