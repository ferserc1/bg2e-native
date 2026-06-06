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

#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/Component.hpp>

namespace bg2e {
namespace scene {

class BG2E_API DrawableComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("Drawable");
    
    DrawableComponent() = default;
    DrawableComponent(std::shared_ptr<DrawableBase> drw) :_drawable { drw } {}
    DrawableComponent(DrawableBase * drw) :_drawable { std::shared_ptr<DrawableBase>(drw) } {}
    virtual ~DrawableComponent() = default;

    std::shared_ptr<DrawableBase> drawableBase();
    [[nodiscard]] const std::shared_ptr<DrawableBase> drawableBase() const;
    std::shared_ptr<Drawable> drawable();
    [[nodiscard]] const std::shared_ptr<Drawable> drawable() const;
    void setDrawable(std::shared_ptr<DrawableBase> drw);

    void draw(
        const glm::mat4& nodeTransform,
        VkCommandBuffer cmd,
        VkPipelineLayout layout,
        std::function<std::vector<VkDescriptorSet>(render::MaterialBase *, const glm::mat4&, uint32_t submIndex)> cb,
        VkPipelineBindPoint bp = VK_PIPELINE_BIND_POINT_GRAPHICS
    );

    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, render::Engine& engine) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

protected:
    std::shared_ptr<DrawableBase> _drawable;
};

}
}
