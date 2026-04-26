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

void PolarTransformControllerComponent::update(float /* delta */)
{
    auto transform = ownerNode()->transform();

    if (transform && _enabled)
    {
        transform->setMatrix(glm::mat4{1.0f});

        transform->translate(_target);

        transform->rotate(glm::radians(_azimuth), 0.0f, 1.0f, 0.0f);
        transform->rotate(glm::radians(_elevation), 1.0f, 0.0f, 0.0f);

        transform->translate(0.0f, 0.0f, _distance);
    }
}

void PolarTransformControllerComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)
{
    if (!jsonData || !jsonData->isObject())
        return;

    json::JsonObject& obj = jsonData->objectValue();

    if (obj.count("azimuth"))
        setAzimuth(obj["azimuth"]->numberValue(_azimuth));

    if (obj.count("elevation"))
        setElevation(obj["elevation"]->numberValue(_elevation));

    if (obj.count("distance"))
        setDistance(obj["distance"]->numberValue(_distance));

    if (obj.count("target"))
    {
        auto targetNode = obj["target"];
        if (targetNode && targetNode->isVec3())
        {
            _target = targetNode->glmVec3Value();
        }
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

    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(PolarTransformControllerComponent);

}
