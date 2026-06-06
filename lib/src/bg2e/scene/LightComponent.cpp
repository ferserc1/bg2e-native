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

#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/TransformVisitor.hpp>

#include "bg2e/scene/Scene.hpp"

namespace bg2e::scene {

const glm::vec3 LightComponent::position() const
{
    glm::vec3 pos { 0.0f, 0.0f, 0.0f };
    
    if (ownerNode())
    {
        pos = ownerNode()->worldMatrix() * glm::vec4(pos, 1.0);
    }
    
    return pos;
}

const glm::vec3 LightComponent::direction() const
{
    glm::vec3 dir { 0.0f, 0.0f, 1.0f };
    
    if (ownerNode())
    {
        auto rotation = glm::mat3(ownerNode()->worldMatrix());
        dir = rotation * dir;
    }
    
    return dir;
}

void LightComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& /* basePath */, [[maybe_unused]] render::Engine& engine)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("lightData"))
    {
        _light.deserialize(obj["lightData"]);
    }
}

std::shared_ptr<json::JsonNode> LightComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    
    auto lightData = _light.serialize();
    obj["lightData"] = lightData;
    
    return compData;
}

void LightComponent::didAddToNode(Node* owner)
{
    auto scene = owner->scene();
    if (scene)
    {
        scene->updateLights();
    }
}

void LightComponent::didRemoveFromNode(Node* prevOwner)
{
    auto scene = prevOwner->scene();
    if (scene)
    {
        scene->updateLights();
    }
}

BG2E_SCENE_REGISTER_COMPONENT(LightComponent);

}
