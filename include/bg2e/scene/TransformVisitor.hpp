//
//  TransformVisitor.hpp
#pragma once

#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e {
namespace scene {

class TransformVisitor : public NodeVisitor {
public:

    static glm::mat4 getWorldMatrix(Node * node)
    {
        TransformVisitor visitor;
        node->acceptReverse(&visitor);
        return visitor._worldMatrix;
    }
    
    void visit(Node * node)
    {
        auto transform = node->getComponent<TransformComponent>();
        if (transform)
        {
            _worldMatrix = _worldMatrix * transform->transform();
        }
    }
    
protected:
    glm::mat4 _worldMatrix { 1.0f };
};

}
}
