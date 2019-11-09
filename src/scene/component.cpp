
#include <bg2e/scene/component.hpp>

#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {

    ComponentRegistry::ComponentRegistry() {}
    ComponentRegistry::~ComponentRegistry() {}

    ComponentRegistry * ComponentRegistry::s_singleton = nullptr;

    Component::Component()
    {

    }

    Component::~Component() {

    }

    // Node * Component::node() {
    //     return dynamic_cast<Node*>(sceneObject());
    // }

    
    Component * Component::Factory(bg2e::db::json::Value * componentData, const bg2e::base::path & resourcePath) {
        using namespace bg2e::db::json;
        std::string type = Value::String((*componentData)["type"]);
        ptr<Component> component = ComponentRegistry::Get()->instantiate(type);
        if (component.valid()) {
            component->deserialize(componentData, resourcePath);
        }
        return component.release();
    }

    
}
}
