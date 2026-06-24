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

#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/app/Mouse.hpp>
#include <bg2e/math/tools.hpp>
#include <bg2e/geo/AABoundingBox.hpp>
#include <algorithm>

namespace bg2e::scene {

void OrbitCameraComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&, [[maybe_unused]] render::Engine& engine)
{
    if (!jsonData || !jsonData->isObject())
    {
        return;
    }

    auto& obj = jsonData->objectValue();

    // Deserialize rotate buttons
    if (obj.count("rotateButtons") && obj["rotateButtons"]->isObject())
    {
        auto& btn = obj["rotateButtons"]->objectValue();
        _rotationButtons.left = btn.count("left") ? btn["left"]->boolValue() : false;
        _rotationButtons.middle = btn.count("middle") ? btn["middle"]->boolValue() : false;
        _rotationButtons.right = btn.count("right") ? btn["right"]->boolValue() : false;
    }

    // Deserialize pan buttons (note: serialized as "panButtonsButtons" in some versions)
    if (obj.count("panButtons") && obj["panButtons"]->isObject())
    {
        auto& btn = obj["panButtons"]->objectValue();
        _panButtons.left = btn.count("left") ? btn["left"]->boolValue() : false;
        _panButtons.middle = btn.count("middle") ? btn["middle"]->boolValue() : false;
        _panButtons.right = btn.count("right") ? btn["right"]->boolValue() : false;
    }
    else if (obj.count("panButtonsButtons") && obj["panButtonsButtons"]->isObject())
    {
        auto& btn = obj["panButtonsButtons"]->objectValue();
        _panButtons.left = btn.count("left") ? btn["left"]->boolValue() : false;
        _panButtons.middle = btn.count("middle") ? btn["middle"]->boolValue() : false;
        _panButtons.right = btn.count("right") ? btn["right"]->boolValue() : false;
    }

    // Deserialize zoom buttons
    if (obj.count("zoomButtons") && obj["zoomButtons"]->isObject())
    {
        auto& btn = obj["zoomButtons"]->objectValue();
        _zoomButtons.left = btn.count("left") ? btn["left"]->boolValue() : false;
        _zoomButtons.middle = btn.count("middle") ? btn["middle"]->boolValue() : false;
        _zoomButtons.right = btn.count("right") ? btn["right"]->boolValue() : false;
    }

    // Deserialize rotation
    if (obj.count("rotation") && obj["rotation"]->isVec2())
    {
        _rotation = obj["rotation"]->glmVec2Value();
    }

    // Deserialize distance
    if (obj.count("distance"))
    {
        _distance = obj["distance"]->numberValue(_distance);
    }

    // Deserialize center
    if (obj.count("center") && obj["center"]->isVec3())
    {
        _center = obj["center"]->glmVec3Value();
    }

    // Deserialize rotation speed
    if (obj.count("rotationSpeed"))
    {
        _rotationSpeed = obj["rotationSpeed"]->numberValue(_rotationSpeed);
    }

    // Deserialize wheel speed
    if (obj.count("wheelSpeed"))
    {
        _wheelSpeed = obj["wheelSpeed"]->numberValue(_wheelSpeed);
    }

    // Deserialize min focus
    if (obj.count("minFocus"))
    {
        _minFocus = obj["minFocus"]->numberValue(_minFocus);
    }

    // Deserialize pitch limits
    if (obj.count("minPitch"))
    {
        _minPitch = obj["minPitch"]->numberValue(_minPitch);
    }

    if (obj.count("maxPitch"))
    {
        _maxPitch = obj["maxPitch"]->numberValue(_maxPitch);
    }

    // Deserialize distance limits
    if (obj.count("minDistance"))
    {
        _minDistance = obj["minDistance"]->numberValue(_minDistance);
    }

    if (obj.count("maxDistance"))
    {
        _maxDistance = obj["maxDistance"]->numberValue(_maxDistance);
    }

    // Deserialize bounds
    if (obj.count("maxX"))
    {
        _maxX = obj["maxX"]->numberValue(_maxX);
    }

    if (obj.count("minX"))
    {
        _minX = obj["minX"]->numberValue(_minX);
    }

    if (obj.count("maxY"))
    {
        _maxY = obj["maxY"]->numberValue(_maxY);
    }

    if (obj.count("minY"))
    {
        _minY = obj["minY"]->numberValue(_minY);
    }

    if (obj.count("maxZ"))
    {
        _maxZ = obj["maxZ"]->numberValue(_maxZ);
    }

    if (obj.count("minZ"))
    {
        _minZ = obj["minZ"]->numberValue(_minZ);
    }

    // Deserialize displacement speed
    if (obj.count("displacementSpeed"))
    {
        _displacementSpeed = obj["displacementSpeed"]->numberValue(_displacementSpeed);
    }

    // Deserialize enabled
    if (obj.count("enabled"))
    {
        _enabled = obj["enabled"]->boolValue(_enabled);
    }
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

void OrbitCameraComponent::update(float /* delta */)
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
            math::BasisVectors basis(transform->matrix(), true);

            glm::vec3 displacement(0.0f);

            if (_keys.w) displacement += basis.forward;
            if (_keys.s) displacement -= basis.forward;
            if (_keys.a) displacement -= basis.right;
            if (_keys.d) displacement += basis.right;
            if (_keys.e) displacement += glm::vec3(0.0f, 1.0f, 0.0f);
            if (_keys.q) displacement -= glm::vec3(0.0f, 1.0f, 0.0f);

            if (glm::length(displacement) > 0.0f)
            {
                displacement = glm::normalize(displacement) * _displacementSpeed;
                _center += displacement;
            }
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

void OrbitCameraComponent::mouseButtonDown(int /*button*/, int x, int y)
{
    if (!_enabled) return;
    _mouseButtonPressed = true;
    _lastPos = { static_cast<float>(x), static_cast<float>(y) };
}

void OrbitCameraComponent::mouseButtonUp(int /*button*/, int /*x*/, int /*y*/)
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
            auto speedFactor = _distance * 0.005f * _panSpeed;
            _distance += delta.x * speedFactor;
            break;
        }
        case OrbitAction::None:
            break;
        }
    }
}

void OrbitCameraComponent::mouseWheel(int /*deltaX*/, int deltaY)
{
    if (!_enabled) return;
    _distance += deltaY * 0.1f * std::clamp(_distance, 0.1f, 5.0f) * _wheelSpeed;
}

void OrbitCameraComponent::keyDown(const app::KeyEvent& event)
{
    if (!_enabled) return;
    
Qu    bool wasAnyKeyPressed = _keys.w || _keys.a || _keys.s || _keys.d || _keys.q || _keys.e || _keys.space;
    
    switch (event.key()) {
        case app::KeyEvent::KeyW: _keys.w = true; break;
        case app::KeyEvent::KeyA: _keys.a = true; break;
        case app::KeyEvent::KeyS: _keys.s = true; break;
        case app::KeyEvent::KeyD: _keys.d = true; break;
        case app::KeyEvent::KeyQ: _keys.q = true; break;
        case app::KeyEvent::KeyE: _keys.e = true; break;
        case app::KeyEvent::KeySpace: _keys.space = true; break;
        default: break;
    }
    
    if (!wasAnyKeyPressed && !_isFlying)
    {
        _isFlying = true;
        _savedDistance = _distance;

        auto rotMatrix = glm::rotate(glm::mat4{1.0f}, glm::radians(_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotMatrix = glm::rotate(rotMatrix, glm::radians(_rotation.x), glm::vec3(-1.0f, 0.0f, 0.0f));
        auto forward = glm::vec3(rotMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));

        _center += forward * (_distance - _flightDistance);
        _distance = _flightDistance;
    }
}

void OrbitCameraComponent::keyUp(const app::KeyEvent& event)
{
    switch (event.key()) {
        case app::KeyEvent::KeyW: _keys.w = false; break;
        case app::KeyEvent::KeyA: _keys.a = false; break;
        case app::KeyEvent::KeyS: _keys.s = false; break;
        case app::KeyEvent::KeyD: _keys.d = false; break;
        case app::KeyEvent::KeyQ: _keys.q = false; break;
        case app::KeyEvent::KeyE: _keys.e = false; break;
        case app::KeyEvent::KeySpace: _keys.space = false; break;
        default: break;
    }
    
    bool anyKeyPressed = _keys.w || _keys.a || _keys.s || _keys.d || _keys.q || _keys.e || _keys.space;
    
    if (!anyKeyPressed && _isFlying)
    {
        _isFlying = false;
        
        if (_savedDistance != _distance)
        {
            auto rotMatrix = glm::rotate(glm::mat4{1.0f}, glm::radians(_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            rotMatrix = glm::rotate(rotMatrix, glm::radians(_rotation.x), glm::vec3(-1.0f, 0.0f, 0.0f));
            auto forward = glm::vec3(rotMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
            
            _center += forward * (_flightDistance - _savedDistance);
            _distance = _savedDistance;
        }
    }
}

void OrbitCameraComponent::reset()
{
    _rotation = _initialRotation;
    _distance = _initialDistance;
    _center = _initialCenter;
}

void OrbitCameraComponent::centerOnTarget(bg2e::scene::Node *target)
{
    if (!target)
    {
        reset();
    }
    else
    {
        _distance = _initialDistance;
        bg2e::scene::Drawable * drawable;
        if (target->drawable() &&
            ((drawable = target->drawable()->drawable().get()))
        ) {
            geo::AABoundingBox bbox(drawable->mesh());
            if (bbox.isValid())
            {
                _distance = std::max({ bbox.max().x, bbox.max().y, bbox.max().z }) * 2.0f;
            }
        }

        _rotation.x = 45.0f;
        _rotation.y = 45.0f;
        _center = target->worldPosition();
    }
}

OrbitCameraComponent::OrbitAction OrbitCameraComponent::getOrbitAction()
{
    if (matchMouseState(_rotationButtons))
    {
        return OrbitAction::Rotate;
    }
    else if (matchMouseState(_panButtons))
    {
        return OrbitAction::Pan;
    }
    else if (matchMouseState(_zoomButtons))
    {
        return OrbitAction::Zoom;
    }
    return OrbitAction::None;
}

BG2E_SCENE_REGISTER_COMPONENT(OrbitCameraComponent);
    
}
