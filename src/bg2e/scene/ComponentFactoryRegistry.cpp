//
//  ComponentFactoryRegistry.cpp
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <iostream>

namespace bg2e::scene {

ComponentFactoryRegistry _componentFactoryRegistrySingleton;

ComponentFactoryRegistry& ComponentFactoryRegistry::get()
{
    return _componentFactoryRegistrySingleton;
}
    
void ComponentFactoryRegistry::registerComponent(const std::string& componentName, Creator creator)
{
    _registry[componentName] = creator;
}

Component* ComponentFactoryRegistry::create(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath)
{
    if (!data->isObject())
    {
        throw std::runtime_error("ComponentFactoryRegistry::create(): invalid JSON data. Expecting object");
    }
    
    std::string componentType = data->stringValue();
    if (_registry.find(componentType) == _registry.end()) {
        std::cout << "WARN: component type not found: " << componentType << std::endl;
        return nullptr;
    }
    
    auto result = _registry[componentType](data, basePath);

    return result;
}

}
