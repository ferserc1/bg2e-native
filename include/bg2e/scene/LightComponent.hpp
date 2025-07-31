//
//  LightComponent.hpp
#pragma once

#include <bg2e/base/Light.hpp>
#include <bg2e/scene/Component.hpp>


namespace bg2e {
namespace scene {

class BG2E_API LightComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("LightSource");
    
    LightComponent() = default;
    virtual ~LightComponent() = default;
    
    const base::Light& light() const { return _light; }
    base::Light& light() { return _light; }
    
    const glm::vec3 position() const;
    const glm::vec3 direction() const;

    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

protected:
    base::Light _light;
};

}
}
