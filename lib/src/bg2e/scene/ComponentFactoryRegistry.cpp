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
#include <stdexcept>

namespace bg2e::scene {

ComponentFactoryRegistry* ComponentFactoryRegistry::_registrySingleton = nullptr;

ComponentFactoryRegistry& ComponentFactoryRegistry::get()
{
    if (_registrySingleton == nullptr)
    {
        _registrySingleton = new ComponentFactoryRegistry();
    }
    return *_registrySingleton;
}

ComponentFactoryRegistry::RegistrationResult ComponentFactoryRegistry::registerComponent(
    const std::string& componentName,
    DefaultCreator defaultCreator,
    Owner owner)
{
    if (componentName.empty() || !defaultCreator)
    {
        return RegistrationResult::Invalid;
    }
    return _registry.emplace(componentName, FactoryEntry{std::move(defaultCreator), owner}).second
        ? RegistrationResult::Registered : RegistrationResult::Duplicate;
}

bool ComponentFactoryRegistry::remove(const std::string& name, Owner owner)
{
    const auto it = _registry.find(name);
    if (!owner || it == _registry.end() || it->second.owner != owner)
    {
        return false;
    }
    _registry.erase(it);
    return true;
}

void ComponentFactoryRegistry::removeOwner(Owner owner)
{
    if (owner)
    {
        std::erase_if(_registry, [owner](const auto& entry)
        {
            return entry.second.owner == owner;
        });
    }
}

std::vector<std::string> ComponentFactoryRegistry::componentNames(Owner owner) const
{
    std::vector<std::string> result;
    for (const auto& [name, entry] : _registry)
    {
        if (entry.owner == owner)
        {
            result.push_back(name);
        }
    }
    return result;
}

ComponentFactoryRegistry::Owner ComponentFactoryRegistry::owner(const std::string& name) const
{
    const auto it = _registry.find(name);
    return it == _registry.end() ? 0 : it->second.owner;
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
    // Keep the callable alive if a constructor causes a registry mutation.
    auto factory = it->second.createDefault;
    return factory();
}

Component* ComponentFactoryRegistry::create(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath)
{
    json::ObjectReader reader(data);
    auto type = reader.getString("type");
    if (!reader.isValid() || !type || type->empty())
    {
        bg2e_log_warning << "Skipping component: missing or invalid type/representation" << bg2e_log_end;
        return nullptr;
    }
    auto it = _registry.find(*type);
    if (it == _registry.end())
    {
        bg2e_log_warning << "Skipping unregistered component type: " << *type << bg2e_log_end;
        return nullptr;
    }
    if (!it->second.createDefault)
    {
        bg2e_log_warning << "Skipping component without a schema constructor: " << *type << bg2e_log_end;
        return nullptr;
    }
    auto factory = it->second.createDefault;
    std::unique_ptr<Component> result(factory());
    if (!result)
    {
        throw std::runtime_error("Component constructor returned null for " + *type);
    }
    if (result->ownerNode() || result->typeName() != *type)
    {
        throw std::logic_error("Schema constructor must return a detached component of type " + *type);
    }
    result->deserialize(data, basePath);
    return result.release();
}

Component* ComponentFactoryRegistry::create(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath,
    draw::Engine& engine, SceneLoadProgress* progress)
{
    std::unique_ptr<Component> result(create(data, basePath));
    if (!result)
    {
        return nullptr;
    }
    result->loadResources(engine, progress);
    return result.release();
}

}
