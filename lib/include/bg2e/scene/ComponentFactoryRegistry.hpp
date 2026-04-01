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

#include <bg2e/scene/Component.hpp>

#include <unordered_map>
#include <string>
#include <functional>
#include <filesystem>

namespace bg2e {
namespace scene {

class BG2E_API ComponentFactoryRegistry {
public:
    using Creator = std::function<Component*(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)>;
    
    static ComponentFactoryRegistry& get();
    
    void registerComponent(const std::string& componentName, Creator creator);
    
    Component* create(std::shared_ptr<json::JsonNode> data, const std::filesystem::path&);

private:
    std::unordered_map<std::string, Creator> _registry;
    static ComponentFactoryRegistry * _registrySingleton;
};

template <typename ComponentT>
class RegisterComponent {
public:
    RegisterComponent() {
        ComponentFactoryRegistry::get().registerComponent(ComponentT::staticTypeName(), [](std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& path) {
            auto result = new ComponentT();
            result->deserialize(jsonData, path);
            return result;
        });
    }
};

}
}

#define BG2E_SCENE_REGISTER_COMPONENT(ComponentClass) \
    RegisterComponent<ComponentClass> _registerComponentInstance##ComponentClass;
    
