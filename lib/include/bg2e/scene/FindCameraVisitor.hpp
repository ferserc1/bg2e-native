//
//  FindCameraVisitor.hpp
#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/CameraComponent.hpp>

#include <vector>

namespace bg2e {
namespace scene {

class BG2E_API FindCameraVisitor : public NodeVisitor {
public:
    void findCameras(Node *);

    void visit(Node*);
    
    void cleanup();
    
    const std::vector<CameraComponent*>& cameras() const { return _cameras; }

protected:
    
    std::vector<CameraComponent*> _cameras;
};

}
}
