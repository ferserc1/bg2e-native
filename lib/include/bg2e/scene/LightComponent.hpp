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

    std::shared_ptr<Component> clone() const override;

    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, render::Engine& engine) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

protected:
    base::Light _light;

    void didAddToNode(Node*) override;
    void didRemoveFromNode(Node*) override;
};

}
}
