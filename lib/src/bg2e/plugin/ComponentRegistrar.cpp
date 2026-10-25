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

#include <bg2e/plugin/ComponentRegistrar.hpp>

#include "ModuleState.hpp"

#include <utility>

namespace bg2e::plugin {

ModuleHandle::ModuleHandle(std::shared_ptr<detail::ModuleState> state)
    : _state(std::move(state))
{
}

PluginId ModuleHandle::id() const noexcept
{
    return _state ? _state->id : 0;
}

bool ModuleHandle::acceptingWork() const noexcept
{
    return _state && _state->accepting.load();
}

ModuleHandle::operator bool() const noexcept
{
    return bool(_state);
}

ComponentRegistrar::ComponentRegistrar(ModuleHandle module, LoadReport& report,
    std::vector<std::function<void()>>& stopCallbacks,
    std::vector<std::string>& componentTypes)
    : _module(std::move(module)), _report(report), _stopCallbacks(stopCallbacks),
      _componentTypes(componentTypes)
{
}

ModuleHandle ComponentRegistrar::module() const
{
    return _module;
}

ComponentRegistrar::Result ComponentRegistrar::onStop(std::function<void()> callback)
{
    if (!callback || !_module.acceptingWork())
    {
        return record("stop callback", Result::Invalid);
    }
    _stopCallbacks.push_back(std::move(callback));
    return Result::Registered;
}

ComponentRegistrar::Result ComponentRegistrar::record(const std::string& type, Result result)
{
    if (result == Result::Duplicate)
    {
        _report.duplicates.push_back(type);
    }
    else if (result == Result::Invalid)
    {
        _report.errors.push_back("Invalid contribution: " + type);
    }
    return result;
}

ComponentRegistrar::Result ComponentRegistrar::registerComponent(const std::string& type,
    scene::ComponentFactoryRegistry::DefaultCreator factory,
    std::optional<reflection::TypeInfo> metadata)
{
    using Registry = scene::ComponentFactoryRegistry;
    if (!_module.acceptingWork() || type.empty() || !factory ||
        (metadata && metadata->typeName != type))
    {
        return record(type, Result::Invalid);
    }
    auto& registry = Registry::get();
    auto& reflectionRegistry = reflection::TypeRegistry::get();
    if (registry.contains(type) || (metadata && reflectionRegistry.contains(type)))
    {
        return record(type, Result::Duplicate);
    }
    auto module = _module;
    auto result = registry.registerComponent(type,
        [module, factory = std::move(factory)]() -> scene::Component*
        {
            if (!module.acceptingWork())
            {
                return nullptr;
            }
            return factory();
        }, _module.id());
    if (result != Result::Registered)
    {
        return record(type, result);
    }
    try
    {
        if (metadata)
        {
            const auto reflected = reflectionRegistry.registerOwnedType(std::move(*metadata), _module.id());
            if (reflected != reflection::TypeRegistry::RegistrationResult::Registered)
            {
                registry.remove(type, _module.id());
                return record(type, static_cast<Result>(reflected));
            }
        }
        // Record only fully committed factory/reflection contributions.
        _componentTypes.push_back(type);
    }
    catch (...)
    {
        if (metadata)
        {
            reflectionRegistry.removeType(type, _module.id());
        }
        registry.remove(type, _module.id());
        throw;
    }
    return result;
}

ComponentRegistrar::Result ComponentRegistrar::registerReflection(reflection::TypeInfo metadata)
{
    const auto type = metadata.typeName;
    if (!_module.acceptingWork())
    {
        return record(type, Result::Invalid);
    }
    return record(type, static_cast<Result>(
        reflection::TypeRegistry::get().registerOwnedType(std::move(metadata), _module.id())));
}

}
