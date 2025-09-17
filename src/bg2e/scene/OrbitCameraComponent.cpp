//
//  OrbitCameraComponent.cpp

#import <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/app/InputManager.hpp>
#include <bg2e/math/tools.hpp>

namespace bg2e::scene {

void OrbitCameraComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)
{

}

std::shared_ptr<json::JsonNode> OrbitCameraComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    
    obj["rotateButtons"] = JSON(JsonObject{
        {"left", JSON(_rotationButtons.left)},
        {"middle", JSON(_rotationButtons.middle)},
        {"right", JSON(_rotationButtons.right)}
    });
    
    obj["panButtonsButtons"] = JSON(JsonObject{
        {"left", JSON(_panButtons.left)},
        {"middle", JSON(_panButtons.middle)},
        {"right", JSON(_panButtons.right)}
    });
    
    obj["zoomButtons"] = JSON(JsonObject{
        {"left", JSON(_zoomButtons.left)},
        {"middle", JSON(_zoomButtons.middle)},
        {"right", JSON(_zoomButtons.right)}
    });
    
    obj["rotation"] = JSON(_rotation);
    obj["distance"] = JSON(_distance);
    obj["center"] = JSON(_center);
    obj["rotationSpeed"] = JSON(_rotationSpeed);
    obj["forward"] = JSON(_forward);
    obj["left"] = JSON(_left);
    obj["wheelSpeed"] = JSON(_wheelSpeed);
    obj["minFocus"] = JSON(_minFocus);
    obj["minPitch"] = JSON(_minPitch);
    obj["maxPitch"] = JSON(_maxPitch);
    obj["minDistance"] = JSON(_minDistance);
    obj["maxDistance"] = JSON(_maxDistance);
    obj["maxX"] = JSON(_maxX);
    obj["minX"] = JSON(_minX);
    obj["maxY"] = JSON(_maxY);
    obj["minY"] = JSON(_minY);
    obj["maxZ"] = JSON(_maxZ);
    obj["minZ"] = JSON(_minZ);
    obj["displacementSpeed"] = JSON(_displacementSpeed);
    obj["enabled"] = JSON(_enabled);
    
    return compData;
}

void OrbitCameraComponent::update(float delta)
{
    auto transform = ownerNode()->transform();
    
    if (transform && _enabled)
    {
    // TODO: Convert this code from JavaScript version
//        let forward = this.transform.matrix.forwardVector;
//        let left = this.transform.matrix.leftVector;
//        forward.scale(this._forward);
//        left.scale(this._left);
//        this._center = Vec.Add(Vec.Add(this._center, forward), left);
//        
//        let pitch = this._rotation.x>this._minPitch ? this._rotation.x:this._minPitch;
//        pitch = pitch<this._maxPitch ? pitch : this._maxPitch;
//        this._rotation.x = pitch;
//
//        this._distance = this._distance>this._minDistance ? this._distance:this._minDistance;
//        this._distance = this._distance<this._maxDistance ? this._distance:this._maxDistance;
//
//        if (this._mouseButtonPressed) {
//            let displacement = new Vec([0,0,0]);
//            if (this._keys[SpecialKey.UP_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.backwardVector);
//            }
//            if (this._keys[SpecialKey.DOWN_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.forwardVector);
//            }
//            if (this._keys[SpecialKey.LEFT_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.leftVector);
//            }
//            if (this._keys[SpecialKey.RIGHT_ARROW]) {
//                displacement = Vec.Add(displacement, this.transform.matrix.rightVector);
//            }
//            displacement.scale(this._displacementSpeed);
//            this._center = Vec.Add(this._center, displacement);
//        }
//
//        if (this._center.x<this._minX) this._center.x = this._minX;
//        else if (this._center.x>this._maxX) this._center.x = this._maxX;
//
//        if (this._center.y<this._minY) this._center.y = this._minY;
//        else if (this._center.y>this._maxY) this._center.y = this._maxY;
//
//        if (this._center.z<this._minZ) this._center.z = this._minZ;
//        else if (this._center.z>this._maxZ) this._center.z = this._maxZ;
//
//        
//        this.transform.matrix.identity();
//
//        if (orthoStrategy) {
//            orthoStrategy.viewWidth = this._viewWidth;
//        }
//        else {
//            this.transform.matrix.translate(0,0,this._distance);
//            if (this.camera) {
//                // Update the camera focus distance to optimize the shadow map rendering
//                this.camera.focusDistance = this._distance;
//            }
//        }
//        this.transform.matrix.rotate(degreesToRadians(pitch), -1,0,0)
//                    .rotate(degreesToRadians(this._rotation.y), 0,1,0)
//                    .translate(this._center);
    }
}

void OrbitCameraComponent::mouseButtonDown(int button, int x, int y)
{
    if (!_enabled) return;
    _mouseButtonPressed = true;
    _lastPos = { static_cast<float>(x), static_cast<float>(y) };
}

void OrbitCameraComponent::mouseButtonUp(int button, int x, int y)
{
    if (!_enabled) return;
    _mouseButtonPressed = false;
}

void OrbitCameraComponent::mouseMove(int x, int y)
{
    if (!_enabled) return;
    auto transform = ownerNode()->transform();
    
    if (transform && _enabled)
    {
        glm::vec2 delta = {
            _lastPos.y - static_cast<float>(y),
            _lastPos.x - static_cast<float>(x)
        };
        _lastPos = { static_cast<float>(x), static_cast<float>(y) };
        auto basis = math::BasisVectors(transform->matrix());
        
        switch (getOrbitAction())
        {
        case OrbitAction::Rotate:
            delta.x = delta.x * -1;
            _rotation = _rotation + delta * 0.5f;
            break;
        case OrbitAction::Pan: {
            auto up = basis.up * delta.x * 0.001f * _distance;
            auto right = basis.right * delta.y * 0.001f * _distance;
            
            _center = _center + up + right;
            break;
        }
        case OrbitAction::Zoom:
            _distance += delta.x * 0.01 * _distance;
            break;
        case OrbitAction::None:
            break;
        }
    }
}

void OrbitCameraComponent::mouseWheel(int deltaX, int deltaY)
{
    if (!_enabled) return;
    auto mult = _distance > 0.01f ? _distance : 0.01;
    _distance += deltaY * 0.001f * mult * _wheelSpeed;
}



OrbitCameraComponent::OrbitAction OrbitCameraComponent::getOrbitAction()
{
    auto mouseState = app::InputManager::getMouseStatus();
    
    if (matchMouseState(mouseState, _rotationButtons))
    {
        return OrbitAction::Rotate;
    }
    else if (matchMouseState(mouseState, _panButtons))
    {
        return OrbitAction::Pan;
    }
    else if (matchMouseState(mouseState, _zoomButtons))
    {
        return OrbitAction::Zoom;
    }
    return OrbitAction::None;
}

BG2E_SCENE_REGISTER_COMPONENT(OrbitCameraComponent);
    
}
