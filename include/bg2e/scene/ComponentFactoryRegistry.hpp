//
//  ComponentFactoryRegistry.hpp
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
    using Creator = std::function<Component*(const std::string& jsonData, const std::filesystem::path&)>;
    
    static ComponentFactoryRegistry& get();
    
    void registerComponent(const std::string& componentName, Creator creator);
    
    Component* create(const std::string& jsonData, const std::filesystem::path&);

private:
    std::unordered_map<std::string, Creator> _registry;
};

template <typename ComponentT>
class RegisterComponent {
public:
    RegisterComponent() {
        ComponentFactoryRegistry::get().registerComponent(ComponentT::componentName(), [](const std::string& jsonData, const std::filesystem::path& path) {
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
    
