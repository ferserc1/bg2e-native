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
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/base/Log.hpp>

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
    
void ComponentFactoryRegistry::registerComponent(
    const std::string& componentName,
    Creator creator,
    DefaultCreator defaultCreator
)
{
    _registry[componentName] = { std::move(creator), std::move(defaultCreator) };
}

bool ComponentFactoryRegistry::contains(const std::string& componentName) const
{
    return _registry.find(componentName) != _registry.end();
}

Component* ComponentFactoryRegistry::createDefault(const std::string& componentName) const
{
    auto it = _registry.find(componentName);
    if (it == _registry.end())
    {
        bg2e_log_warning << "component type not found: " << componentName << bg2e_log_end;
        return nullptr;
    }
    return it->second.createDefault();
}

Component* ComponentFactoryRegistry::create(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath, render::Engine& engine)
{
    return create(data, basePath, engine, nullptr);
}

Component* ComponentFactoryRegistry::create(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath, render::Engine& engine, SceneLoadProgress* progress)
{
    json::ObjectReader reader(data);
    if (!reader.isValid())
    {
        bg2e_log_warning << "Skipping component: expected a JSON object" << bg2e_log_end;
        return nullptr;
    }

    auto componentType = reader.getString("type");
    if (!componentType || componentType->empty())
    {
        bg2e_log_warning << "Skipping component: missing or invalid type" << bg2e_log_end;
        return nullptr;
    }

    auto it = _registry.find(*componentType);
    if (it == _registry.end())
    {
        bg2e_log_warning << "component type not found: " << *componentType << bg2e_log_end;
        return nullptr;
    }
    else if (bg2e::base::Log::isDebug())
    {
        bg2e_log_debug << "Deserialize component: " << *componentType << bg2e_log_end;
    }

    try
    {
        return it->second.deserialize(data, basePath, engine, progress);
    }
    catch (const std::exception& error)
    {
        bg2e_log_warning << "Skipping component " << *componentType << ": " << error.what() << bg2e_log_end;
        return nullptr;
    }
}

}
