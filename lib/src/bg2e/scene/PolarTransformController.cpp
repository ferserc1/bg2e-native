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

#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <algorithm>
#include <cmath>

namespace bg2e::scene {

std::shared_ptr<Component> PolarTransformControllerComponent::clone() const
{
    auto copy = std::make_shared<PolarTransformControllerComponent>(*this);
    copy->_owner = nullptr;
    return copy;
}

void PolarTransformControllerComponent::setAzimuth(float value)
{
    _azimuth = std::fmod(value, 360.0f);
    if (_azimuth < 0.0f)
    {
        _azimuth += 360.0f;
    }
}

void PolarTransformControllerComponent::setElevation(float value)
{
    _elevation = std::clamp(value, -90.0f, 90.0f);
}

void PolarTransformControllerComponent::setDistance(float value)
{
    _distance = value < 0.0f ? 0.0f : value;
}

void PolarTransformControllerComponent::setEulerX(float value)
{
    _eulerX = value;
}

void PolarTransformControllerComponent::setEulerY(float value)
{
    _eulerY = value;
}

void PolarTransformControllerComponent::setEulerZ(float value)
{
    _eulerZ = value;
}

void PolarTransformControllerComponent::update([[maybe_unused]] float delta)
{
    auto transform = ownerNode()->transform();

    if (transform && _enabled)
    {
        glm::mat4 polarMatrix{1.0f};

        polarMatrix = glm::translate(polarMatrix, _target);
        polarMatrix = glm::rotate(polarMatrix, glm::radians(_azimuth), glm::vec3{0.0f, 1.0f, 0.0f});
        polarMatrix = glm::rotate(polarMatrix, glm::radians(_elevation), glm::vec3{-1.0f, 0.0f, 0.0f});

        glm::vec3 position = glm::vec3(
            polarMatrix * glm::vec4{0.0f, 0.0f, _distance, 1.0f}
        );

        glm::mat4 finalMatrix{1.0f};
        finalMatrix = glm::translate(finalMatrix, position);

        if (_eulerY != 0.0f)
        {
            finalMatrix = glm::rotate(finalMatrix, glm::radians(_eulerY), glm::vec3{0.0f, 1.0f, 0.0f});
        }
        if (_eulerX != 0.0f)
        {
            finalMatrix = glm::rotate(finalMatrix, glm::radians(_eulerX), glm::vec3{1.0f, 0.0f, 0.0f});
        }
        if (_eulerZ != 0.0f)
        {
            finalMatrix = glm::rotate(finalMatrix, glm::radians(_eulerZ), glm::vec3{0.0f, 0.0f, 1.0f});
        }

        transform->setMatrix(finalMatrix);
    }
}

void PolarTransformControllerComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&, [[maybe_unused]] render::Engine& engine)
{
    Component::deserialize(jsonData, {}, engine);

    if (!jsonData || !jsonData->isObject())
        return;

    json::JsonObject& obj = jsonData->objectValue();

    if (obj.count("azimuth"))
    {
        setAzimuth(obj["azimuth"]->numberValue(_azimuth));
    }

    if (obj.count("elevation"))
    {
        setElevation(obj["elevation"]->numberValue(_elevation));
    }

    if (obj.count("distance"))
    {
        setDistance(obj["distance"]->numberValue(_distance));
    }

    if (obj.count("target"))
    {
        auto targetNode = obj["target"];
        if (targetNode && targetNode->isVec3())
        {
            _target = targetNode->glmVec3Value();
        }
    }

    if (obj.count("eulerX"))
    {
        _eulerX = obj["eulerX"]->numberValue(_eulerX);
    }

    if (obj.count("eulerY"))
    {
        _eulerY = obj["eulerY"]->numberValue(_eulerY);
    }

    if (obj.count("eulerZ"))
    {
        _eulerZ = obj["eulerZ"]->numberValue(_eulerZ);
    }
}

std::shared_ptr<json::JsonNode> PolarTransformControllerComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject& obj = compData->objectValue();

    obj["azimuth"] = JSON(_azimuth);
    obj["elevation"] = JSON(_elevation);
    obj["distance"] = JSON(_distance);
    obj["target"] = JSON(_target);
    obj["eulerX"] = JSON(_eulerX);
    obj["eulerY"] = JSON(_eulerY);
    obj["eulerZ"] = JSON(_eulerZ);

    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(PolarTransformControllerComponent);

}
