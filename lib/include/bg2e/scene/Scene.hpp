/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
    
    [[nodiscard]] const std::vector<LightComponent*> lightComponents() const { return _lightComponents; }
    [[nodiscard]] const std::vector<base::Light*> lights() const { return _lights; }
    
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
    std::vector<LightComponent*> _lightComponents;
    std::vector<base::Light*> _lights;

    bool _lightsChanged = false;
};

}
}
