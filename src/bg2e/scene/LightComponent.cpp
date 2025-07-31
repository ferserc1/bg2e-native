//
//  LightComponent.cpp
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/TransformVisitor.hpp>

namespace bg2e::scene {

const glm::vec3 LightComponent::position() const
{
    glm::vec3 pos { 0.0f, 0.0f, 0.0f };
    
    if (ownerNode())
    {
        pos = ownerNode()->worldMatrix() * glm::vec4(pos, 1.0);
    }
    
    return pos;
}

const glm::vec3 LightComponent::direction() const
{
    glm::vec3 dir { 0.0f, 0.0f, 1.0f };
    
    if (ownerNode())
    {
        auto rotation = glm::mat3(ownerNode()->worldMatrix());
        dir = rotation * dir;
    }
    
    return dir;
}

void LightComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{

}

std::shared_ptr<json::JsonNode> LightComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    
    auto lightData = _light.serialize();
    obj["lightData"] = lightData;
    
    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(LightComponent);

}
