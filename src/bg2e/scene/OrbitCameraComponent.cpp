//
//  OrbitCameraComponent.cpp

#include <bg2e/scene/OrbitCameraComponent.hpp>
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

void OrbitCameraComponent::resizeViewport(const math::Viewport& vp)
{
    _viewportWidth = static_cast<uint32_t>(vp.width);
    _viewportHeight = static_cast<uint32_t>(vp.height);
}

void OrbitCameraComponent::update(float delta)
{
    auto transform = ownerNode()->transform();
    
    if (transform && _enabled)
    {
        math::BasisVectors basis(transform->matrix(), true);

        auto pitch = _rotation.x > _minPitch ? _rotation.x : _minPitch;
        pitch = pitch < _maxPitch ? pitch : _maxPitch;
        _rotation.x = pitch;

        // The minimum distance is only restricted if not the minimum float value
        if (_minDistance != std::numeric_limits<float>::min())
        {
            _distance = _distance > std::numeric_limits<float>::epsilon() ? _distance : std::numeric_limits<float>::epsilon();
        }
        _distance = _distance < _maxDistance ? _distance : _maxDistance;

        if (_mouseButtonPressed)
        {
            // TODO: displacement using keyboard arrows
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
            
        }

        if (_center.x < _minX) _center.x = _minX;
        else if (_center.x > _maxX) _center.x = _maxX;
        
        if (_center.y < _minY) _center.y = _minY;
        else if (_center.y > _maxY) _center.y = _maxY;
        
        if (_center.z < _minZ) _center.z = _minZ;
        else if (_center.z > _maxZ) _center.z = _maxZ;

        
        
        transform->setMatrix(glm::mat4{ 1.0f });
        
        transform->translate(_center);
        
        transform->rotate(glm::radians(_rotation.y), 0.0f, 1.0f, 0.0f);
        transform->rotate(glm::radians(pitch), -1.0f, 0.0f, 0.0f);
        
        
        transform->translate(0.0f, 0.0f, _distance);
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
    if (!_mouseButtonPressed) return;
    auto transform = ownerNode()->transform();
    
    if (transform && _enabled)
    {
        glm::vec2 delta = {
            _lastPos.y - static_cast<float>(y),
            _lastPos.x - static_cast<float>(x)
        };
        _lastPos = { static_cast<float>(x), static_cast<float>(y) };
        auto basis = math::BasisVectors(transform->matrix(), true);
        
        switch (getOrbitAction())
        {
        case OrbitAction::Rotate:
            delta.x = delta.x * -1;
            _rotation = _rotation + delta * 0.5f;
            break;
        case OrbitAction::Pan: {
            auto speedFactor = std::abs((std::log(_distance) + 2.0f)) * 0.01f * _panSpeed;
            if (std::isnan(speedFactor))
            {
                speedFactor = 0.01f;
            }
            auto up = basis.up * -delta.x * speedFactor;
            auto right = basis.right * delta.y * speedFactor;
            
            _center = _center + up + right;
            break;
        }
        case OrbitAction::Zoom: {
            auto speedFactor = _distance * 0.001f * _panSpeed;
            _distance += delta.x * speedFactor;
            break;
        }
        case OrbitAction::None:
            break;
        }
    }
}

void OrbitCameraComponent::mouseWheel(int deltaX, int deltaY)
{
    if (!_enabled) return;
    auto mult = _distance > 0.1f ? _distance : 0.1f;
    _distance += deltaY * 0.1f * mult * _wheelSpeed;
}

void OrbitCameraComponent::reset()
{
    _rotation = { 0.0f, 0.0f };
    _distance = 5.0f;
    _center = { 0.0f, 0.0f, 0.0f };
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
