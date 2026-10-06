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
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/app/Mouse.hpp>
#include <bg2e/math/tools.hpp>
#include <bg2e/geo/AABoundingBox.hpp>
#include <algorithm>

namespace bg2e::scene {

std::shared_ptr<Component> OrbitCameraComponent::clone() const
{
    auto copy = std::make_shared<OrbitCameraComponent>(*this);
    copy->_owner = nullptr;
    return copy;
}

void OrbitCameraComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&, [[maybe_unused]] render::Engine& engine)
{
    json::ObjectReader reader(jsonData);
    if (!reader.isValid()) return;

    auto readButtons = [](const json::ObjectReader& source, auto& buttons) {
        buttons.left = source.getBool("left").value_or(false);
        buttons.middle = source.getBool("middle").value_or(false);
        buttons.right = source.getBool("right").value_or(false);
    };

    if (auto buttons = reader.getObject("rotateButtons")) readButtons(*buttons, _rotationButtons);
    if (auto buttons = reader.getObject("panButtons")) readButtons(*buttons, _panButtons);
    else if (auto legacy = reader.getObject("panButtonsButtons")) readButtons(*legacy, _panButtons);
    if (auto buttons = reader.getObject("zoomButtons")) readButtons(*buttons, _zoomButtons);

    if (auto value = reader.getGlmVec2("rotation")) _rotation = *value;
    if (auto value = reader.getGlmVec3("center")) _center = *value;
    if (auto value = reader.getNumber("distance")) _distance = *value;
    if (auto value = reader.getNumber("rotationSpeed")) _rotationSpeed = *value;
    if (auto value = reader.getNumber("wheelSpeed")) _wheelSpeed = *value;
    if (auto value = reader.getNumber("minFocus")) _minFocus = *value;
    if (auto value = reader.getNumber("minPitch")) _minPitch = *value;
    if (auto value = reader.getNumber("maxPitch")) _maxPitch = *value;
    if (auto value = reader.getNumber("minDistance")) _minDistance = *value;
    if (auto value = reader.getNumber("maxDistance")) _maxDistance = *value;
    if (auto value = reader.getNumber("maxX")) _maxX = *value;
    if (auto value = reader.getNumber("minX")) _minX = *value;
    if (auto value = reader.getNumber("maxY")) _maxY = *value;
    if (auto value = reader.getNumber("minY")) _minY = *value;
    if (auto value = reader.getNumber("maxZ")) _maxZ = *value;
    if (auto value = reader.getNumber("minZ")) _minZ = *value;
    if (auto value = reader.getNumber("displacementSpeed")) _displacementSpeed = *value;
    if (auto value = reader.getBool("enabled")) _enabled = *value;
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
    
    bool wasAnyKeyPressed = _keys.w || _keys.a || _keys.s || _keys.d || _keys.q || _keys.e || _keys.space;
    
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
