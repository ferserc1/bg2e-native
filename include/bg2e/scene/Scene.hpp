//
//  Scene.hpp
#pragma once

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>

#include <vector>

namespace bg2e {
namespace scene {

class BG2E_API Scene {
public:
    void setSceneRoot(std::shared_ptr<Node> scene);
    void setMainCamera(CameraComponent * camera);
    
    inline Node * rootNode() { return _sceneRoot.get(); }
    
    CameraComponent * mainCamera();
    EnvironmentComponent * mainEnvironment();
    
    // Call this once in the update() function every time a light is added
    // or removed from the scene
    void updateLights();
    
    inline const std::vector<LightComponent*> lights() const { return _lights; }
    
    // Indicates whether the lights have changed since the last frame. This flag is set to false in didDraw().
    inline bool lightsChanged() const { return _lightsChanged; }
    
    // Render loop events
    void willResize();
    void didResize();
    void willUpdate();
    void didUpdate();
    void willDraw();
    void didDraw();
    
protected:    
    std::shared_ptr<Node> _sceneRoot;
    Node * _mainCameraNode;
    Node * _mainEnvironment;
    std::vector<LightComponent*> _lights;
    bool _lightsChanged = false;
};

}
}
