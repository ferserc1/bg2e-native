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

#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/base/Log.hpp>
#include <iostream>

namespace bg2e::scene {


ComponentFactoryRegistry * ComponentFactoryRegistry::_registrySingleton = nullptr;

ComponentFactoryRegistry& ComponentFactoryRegistry::get()
{
    if (_registrySingleton == nullptr)
    {
        _registrySingleton = new ComponentFactoryRegistry();
    }
    return *_registrySingleton;
}
    
void ComponentFactoryRegistry::registerComponent(const std::string& componentName, Creator creator)
{
    _registry[componentName] = creator;
}

Component* ComponentFactoryRegistry::create(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath, render::Engine& engine)
{
    if (!data->isObject() && !data->isNull())
    {
        throw std::runtime_error("ComponentFactoryRegistry::create(): invalid JSON data. Expecting object");
    }

    if (data->isNull())
    {
        return nullptr;
    }

    auto objectValue = data->objectValue();
    std::string componentType = objectValue["type"]->stringValue();
    if (_registry.find(componentType) == _registry.end())
    {
        bg2e_log_warning << "component type not found: " << componentType << bg2e_log_end;
        return nullptr;
    }
    else if (bg2e::base::Log::isDebug())
    {
        bg2e_log_debug << "Deserialize component: " << componentType << bg2e_log_end;
    }
    
    auto result = _registry[componentType](data, basePath, engine);
    return result;
}

}
