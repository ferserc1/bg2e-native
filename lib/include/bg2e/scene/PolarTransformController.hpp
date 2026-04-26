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

#include <bg2e/scene/Component.hpp>
#include <bg2e/math/base.hpp>

namespace bg2e {
namespace scene {

class BG2E_API PolarTransformControllerComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("PolarTransformController");

    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path&) override;

    inline bool enabled() const { return _enabled; }
    inline void setEnabled(bool value) { _enabled = value; }

    inline float azimuth() const { return _azimuth; }
    inline void setAzimuth(float value);

    inline float elevation() const { return _elevation; }
    inline void setElevation(float value);

    inline float distance() const { return _distance; }
    inline void setDistance(float value);

    inline const glm::vec3& target() const { return _target; }
    inline void setTarget(const glm::vec3& target) { _target = target; }

    void update(float delta) override;

protected:
    float _azimuth = 0.0f;
    float _elevation = 0.0f;
    float _distance = 5.0f;
    glm::vec3 _target = {0.0f, 0.0f, 0.0f};
    bool _enabled = true;
};

}
}
