//
//  DrawableComponent.cpp
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/utils/utils.hpp>
#include <bg2e/db/mesh_bg2.hpp>

#include <iostream>

namespace bg2e::scene {

std::shared_ptr<DrawableBase> DrawableComponent::drawable()
{
    return _drawable;
}

void DrawableComponent::setDrawable(std::shared_ptr<DrawableBase> drw)
{
    _drawable = drw;
}

void DrawableComponent::draw(
    const glm::mat4& nodeTransform,
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb,
    VkPipelineBindPoint bp
) {
    auto drawable = _drawable.get();
    if (drawable)
    {
        drawable->setTransform(nodeTransform);
        drawable->draw(cmd, layout, cb, bp);
    }
}

void DrawableComponent::deserialize(
    std::shared_ptr<json::JsonNode> jsonData,
    const std::filesystem::path& basePath
) {

}

std::shared_ptr<json::JsonNode> DrawableComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    
    Drawable * drawable = dynamic_cast<Drawable*>(_drawable.get());
    if (drawable)
    {
        if (_drawable->name() == "")
        {
            _drawable->setName(utils::uniqueId());
        }
        
        obj["name"] = JSON(_drawable->name());
        
        auto filePath = basePath;
        filePath.append(_drawable->name());
        filePath.replace_extension(".bg2");
        
        db::storeDrawableBg2(filePath, drawable);
    }

    return compData;
}
    
BG2E_SCENE_REGISTER_COMPONENT(DrawableComponent);

}
