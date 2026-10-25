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

#pragma once

#include <bg2e/plugin/Types.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>

#include <optional>
#include <utility>

namespace bg2e::plugin {

// Valid only during registerComponents, on the manager's owning thread.
// Do not retain this object or register through scene/reflection singletons.
class BG2E_API ComponentRegistrar {
public:
    using Result = scene::ComponentFactoryRegistry::RegistrationResult;

    ComponentRegistrar(const ComponentRegistrar&) = delete;
    ComponentRegistrar& operator=(const ComponentRegistrar&) = delete;

    Result registerComponent(const std::string& type,
        scene::ComponentFactoryRegistry::DefaultCreator factory,
        std::optional<reflection::TypeInfo> metadata = std::nullopt);
    template <typename T>
    Result registerComponent(std::optional<reflection::TypeInfo> metadata = std::nullopt)
    {
        return registerComponent(T::staticTypeName(), []
        {
            return new T();
        }, std::move(metadata));
    }

    Result registerReflection(reflection::TypeInfo metadata);

    template <typename Base, typename Derived>
    Result registerSubtype(std::string base, std::string key,
        std::string displayName, std::string type = {})
    {
        const auto label = base + ":" + key;
        if (!_module.acceptingWork())
        {
            return record(label, Result::Invalid);
        }
        auto result = reflection::TypeRegistry::get().registerSubtype<Base, Derived>(
            std::move(base), std::move(key), std::move(displayName), std::move(type), _module.id());
        return record(label, static_cast<Result>(result));
    }

    // Cancel/join module jobs and revoke external subscriptions at preparation.
    // Each callback runs once; failures do not skip subsequent callbacks.
    Result onStop(std::function<void()> callback);

    ModuleHandle module() const;

private:
    friend class PluginManager;
    ComponentRegistrar(ModuleHandle module, LoadReport& report,
        std::vector<std::function<void()>>& stopCallbacks,
        std::vector<std::string>& componentTypes);
    Result record(const std::string& type, Result result);
    ModuleHandle _module;
    LoadReport& _report;
    std::vector<std::function<void()>>& _stopCallbacks;
    std::vector<std::string>& _componentTypes;
};

}
