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

#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/utils/utils.hpp>
#include <bg2e/db/mesh_bg2.hpp>

#include <iostream>

namespace bg2e::scene {

std::shared_ptr<DrawableBase> DrawableComponent::drawableBase()
{
    return _drawable;
}

const std::shared_ptr<DrawableBase> DrawableComponent::drawableBase() const
{
    return _drawable;
}

std::shared_ptr<Drawable> DrawableComponent::drawable()
{
    return std::dynamic_pointer_cast<Drawable>(_drawable);
}

const std::shared_ptr<Drawable> DrawableComponent::drawable() const
{
    return std::dynamic_pointer_cast<Drawable>(_drawable);
}

void DrawableComponent::setDrawable(std::shared_ptr<DrawableBase> drw)
{
    _drawable = drw;
}

std::shared_ptr<Component> DrawableComponent::clone() const
{
    auto copy = std::make_shared<DrawableComponent>();

    // Deep copy the drawable so the duplicated component owns an independent mesh,
    // materials and GPU resources. This is the whole point of cloning by copy: the
    // two DrawableComponents must never share the same DrawableBase instance.
    if (_drawable)
    {
        copy->_drawable = _drawable->clone();
    }

    copy->_priority = _priority;
    copy->_ignoreSerialization = _ignoreSerialization;
    return copy;
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
    const std::filesystem::path& basePath,
    render::Engine& engine
) {
    if (!jsonData || !jsonData->isObject())
    {
        return;
    }

    auto& obj = jsonData->objectValue();

    if (obj.count("name"))
    {
        auto name = obj["name"]->stringValue();

        auto filePath = basePath;
        filePath.append(name);
        filePath.replace_extension(".bg2");

        _drawable = db::loadDrawableBg2(filePath, &engine);
    }
}

void DrawableComponent::deserializeWithProgress(
    std::shared_ptr<json::JsonNode> jsonData,
    const std::filesystem::path& basePath,
    render::Engine& engine,
    SceneLoadProgress* progress
) {
    if (!jsonData || !jsonData->isObject())
    {
        return;
    }

    auto& obj = jsonData->objectValue();

    if (obj.count("name"))
    {
        auto name = obj["name"]->stringValue();

        auto filePath = basePath;
        filePath.append(name);
        filePath.replace_extension(".bg2");

        _drawable = db::loadDrawableBg2(filePath, &engine, [progress]() {
            if (progress && progress->callback)
            {
                progress->callback("texture", progress->loaded, progress->total);
            }
            if (progress) { ++progress->loaded; }
        });
    }
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
