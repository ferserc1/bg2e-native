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

#include <bg2e/common.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/base/Light.hpp>
#include <bg2e/base/Color.hpp>
#include <bg2e/math/base.hpp>

#include <memory>
#include <unordered_map>

namespace bg2e {
namespace manipulation {

enum class GizmoType {
    None,
    Camera,
    PointLight,
    SpotLight,
    DirectionalLight,
    Environment
};

class BG2E_API GizmoComponent : public scene::DrawableComponent {
public:
    BG2E_COMPONENT_TYPE_NAME("Gizmo");

    GizmoComponent(render::Engine* engine);
    virtual ~GizmoComponent() = default;

    // No serialization — gizmos are runtime-only
    void deserialize(std::shared_ptr<json::JsonNode>, const std::filesystem::path&, render::Engine&) override {}
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path&) override { return nullptr; }

    void update(float delta) override;

    GizmoType currentGizmoType() const { return _currentGizmoType; }

    // Returns the light color for light gizmos, white for camera/environment
    base::Color gizmoColor() const;

    // Returns world transform with fixed-screen-size scaling applied
    glm::mat4 renderTransform(
        const glm::mat4& worldTransform,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix
    ) const;

    static float gizmoScale(GizmoType type);
    static void  setGizmoScale(GizmoType type, float scale);

    static float gizmoOpacity(GizmoType type);
    static void  setGizmoOpacity(GizmoType type, float opacity);

    static bool  isGizmoVisible(GizmoType type);
    static void  setGizmoVisible(GizmoType type, bool visible);

    static void cleanupStatic();

private:
    render::Engine* _engine;
    GizmoType _currentGizmoType = GizmoType::None;

    GizmoType resolveGizmoType() const;
    void loadGizmo(GizmoType type);
    void unloadGizmo();

    // Static mesh cache: shared across all GizmoComponent instances
    static std::unordered_map<GizmoType, std::shared_ptr<scene::Drawable>> _gizmoCache;
    static std::shared_ptr<scene::Drawable> getCachedGizmo(GizmoType type, render::Engine* engine);

    static std::unordered_map<GizmoType, float> _gizmoScales;
    static std::unordered_map<GizmoType, float> _gizmoOpacities;
    static std::unordered_map<GizmoType, bool>  _gizmoVisible;

    static bool _cleanupRegistered;
};

}
}
