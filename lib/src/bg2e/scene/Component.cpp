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

#include <bg2e/scene/Component.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <stdexcept>

namespace bg2e::scene {

void Component::setPriority(uint32_t newPriority)
{
    if (_priority != newPriority)
    {
        _priority = newPriority;
        if (_owner)
        {
            _owner->setComponentsOrderDirty();
        }
    }
}

Scene* Component::scene()
{
    return _owner != nullptr ? _owner->scene() : nullptr;
}

std::string componentName(Component* component)
{
    return typeid(*component).name();
}

void Component::deserializeCommon(std::shared_ptr<json::JsonNode> jsonData)
{
    json::ObjectReader reader(jsonData);
    if (auto priority = reader.getInteger<uint32_t>("priority"))
    {
        setPriority(*priority);
    }
}

void Component::deserialize(std::shared_ptr<json::JsonNode>, const std::filesystem::path&)
{
    throw std::logic_error(typeName() + " does not implement graphics-free deserialization");
}

std::shared_ptr<json::JsonNode> Component::serializeReferences(const std::filesystem::path&)
{
    throw std::logic_error(typeName() + " does not implement reference-only serialization");
}

void Component::loadResources(draw::Engine&, SceneLoadProgress*)
{
}

std::shared_ptr<json::JsonNode> Component::serialize(const std::filesystem::path& /* basePath */)
{
    using namespace bg2e::json;
    return JSON(JsonObject{
        {"type", JSON(typeName())},
        {"priority", JSON(static_cast<double>(_priority))}});
}

}
