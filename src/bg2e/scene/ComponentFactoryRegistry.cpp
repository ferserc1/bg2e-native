//
//  ComponentFactoryRegistry.cpp
#include <bg2e/scene/ComponentFactoryRegistry.hpp>

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

Component* ComponentFactoryRegistry::create(const std::string& jsonData, const std::filesystem::path&)
{
    // TODO: Parse json data
    // get component name from json data
    // if (_registry[componentName] != _registry.end()) {
    //      auto result = _registry[componentName]();
    //      result->deserialize(jsonData);
    //      return result;
    // }

    return nullptr;
}

}
