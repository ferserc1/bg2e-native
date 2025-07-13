//
//  Component.cpp
#include <bg2e/scene/Component.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>

namespace bg2e::scene {

Scene * Component::scene()
{
    return _owner != nullptr ? _owner->scene() : nullptr;
}

std::string componentName(Component * component)
{
    return typeid(*component).name();
}

void Component::deserialize(
    std::shared_ptr<json::JsonNode> jsonData,
    const std::filesystem::path& basePath
) {

}

std::shared_ptr<json::JsonNode> Component::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    return JSON(JsonObject{
        { "type", JSON(typeName()) }
    });
}

}
