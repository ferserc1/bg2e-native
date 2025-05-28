//
//  Scene.hpp
#pragma once

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/CameraComponent.hpp>

namespace bg2e {
namespace scene {

class BG2E_API Scene {
public:
    void setSceneRoot(std::shared_ptr<Node> scene);
    void setMainCamera(CameraComponent * camera);
    
    inline Node * rootNode() { return _sceneRoot.get(); }
    
    CameraComponent * mainCamera();
    
protected:
    std::shared_ptr<Node> _sceneRoot;
    Node * _mainCameraNode;
};

}
}
