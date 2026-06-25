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
#include <string>
#include <unordered_map>
#include <vector>

namespace bg2e {
namespace scene {
class Node;
class Scene;
}

namespace manipulation {

enum class GizmoType {
    None,
    Camera,
    PointLight,
    SpotLight,
    DirectionalLight,
    Environment,
    Transform
};

// Identifies which part of the transform gizmo is being interacted with. The
// transformation math is applied in a later step; this only carries intent.
enum class TransformHandle {
    None,
    TranslateX, TranslateY, TranslateZ,
    RotateX, RotateY, RotateZ,
    ScaleX, ScaleY, ScaleZ,
    ScaleUniform
};

class BG2E_API GizmoComponent : public scene::DrawableComponent {
public:
    BG2E_COMPONENT_TYPE_NAME("Gizmo");

    GizmoComponent(render::Engine* engine);
    virtual ~GizmoComponent() = default;

    // Gizmos are runtime-only helpers: a clone is a fresh gizmo for the same
    // engine, not a literal copy of the source's transient interaction state.
    std::shared_ptr<scene::Component> clone() const override;

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

    // Same as above but using the scale configured for an explicit gizmo type.
    // Used to render the transform gizmo facet (GizmoType::Transform), whose
    // scale is independent from the node's type gizmo.
    glm::mat4 renderTransform(
        const glm::mat4& worldTransform,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        GizmoType type
    ) const;

    // --- Transform gizmo facet ---------------------------------------------
    // The transform gizmo is an independent facet that can coexist with the
    // type gizmo above (a light node shows both its light gizmo and, when it is
    // the current transform node, the transform gizmo). It is only visible when
    // this node is the current transform node AND owns a TransformComponent.
    [[nodiscard]] bool transformVisible() const { return _transformVisible; }
    void setTransformVisible(bool visible) { _transformVisible = visible; }

    // Lazily-loaded shared transform gizmo drawable. Returns nullptr until the
    // asset has been loaded (see ensureTransformGizmoLoaded()).
    [[nodiscard]] std::shared_ptr<scene::Drawable> transformDrawable() const;

    // Current transform node management. setCurrentTransform hides the previous
    // transform gizmo, resets the tracked component and, if the given node owns
    // a GizmoComponent, marks it as the current transform and makes its gizmo
    // visible. Only one transform gizmo can be visible at a time.
    static void setCurrentTransform(scene::Node* node);
    static scene::Node* currentTransformNode();

    // Scale sub-control visibility. Uniform (global) and per-axis (component)
    // scale handles can be hidden independently. Rotation and translation
    // controls are always visible. Mirrors the per-type visibility API below.
    static bool isScaleUniformVisible();
    static void setScaleUniformVisible(bool visible);
    static bool isScaleAxisVisible();
    static void setScaleAxisVisible(bool visible);

    // Returns whether a transform-gizmo submesh must be drawn, honoring the
    // scale-control visibility flags. name/groupName come from the submesh.
    static bool isTransformSubmeshVisible(const std::string& name, const std::string& groupName);

    // Maps a transform-gizmo submesh name to the handle it represents, using the
    // asset naming convention (A=X, B=Z, C=Y).
    static TransformHandle handleForSubmesh(const std::string& name, const std::string& groupName);

    // Persistent pick identifier for a transform-gizmo submesh. Allocated lazily
    // from the shared SelectableComponent identifier counter so it never collides
    // with drawable/gizmo identifiers. Requires the transform gizmo to be loaded;
    // returns 0 otherwise.
    uint32_t transformSubmeshIdentifier(uint32_t submeshIndex);

    // Transform interaction. Captures the initial state at beginTransform and
    // recomputes the node's local matrix on every updateTransform call while
    // dragging. endTransform restores the active-handle highlight color.
    void beginTransform(TransformHandle handle, int mouseX, int mouseY);
    void updateTransform(scene::Scene* scene, int mouseX, int mouseY);
    void endTransform();
    [[nodiscard]] TransformHandle activeHandle() const { return _activeHandle; }
    [[nodiscard]] bool isTransforming() const { return _activeHandle != TransformHandle::None; }

    static float gizmoScale(GizmoType type);
    static void  setGizmoScale(GizmoType type, float scale);

    static float gizmoOpacity(GizmoType type);
    static void  setGizmoOpacity(GizmoType type, float opacity);

    static bool  isGizmoVisible(GizmoType type);
    static void  setGizmoVisible(GizmoType type, bool visible);

    // Scale sensitivity: controls how many pixels of mouse movement correspond
    // to a meaningful scale change. Higher values = slower scaling.
    static float gizmoScaleSensitivity(GizmoType type);
    static void  setGizmoScaleSensitivity(GizmoType type, float sensitivity);

    static void cleanupStatic();

private:
    render::Engine* _engine;
    GizmoType _currentGizmoType = GizmoType::None;
    bool _transformVisible = false;

    // Pick identifiers per transform-gizmo submesh (lazily allocated).
    std::vector<uint32_t> _transformSubmeshIds;

    // Currently active transform handle (None when not manipulating).
    TransformHandle _activeHandle = TransformHandle::None;

    // Drag-start state captured at beginTransform
    glm::mat4 _initialLocalMatrix { 1.0f };
    glm::mat3 _parentRotation { 1.0f };
    glm::mat3 _localRotation { 1.0f };
    glm::vec3 _localAxis { 0.0f };
    glm::vec3 _worldAxis { 0.0f };
    glm::vec3 _initialWorldPos { 0.0f };
    glm::vec3 _initialScale { 1.0f };
    glm::vec3 _initialIntersection { 0.0f };
    glm::vec3 _initialAxisPoint { 0.0f };
    float _parentScaleAxis = 1.0f;
    int _mouseDownX = 0;
    int _mouseDownY = 0;
    base::Color _originalHandleColor;

    // Maps a TransformHandle to its local-axis index (0=X, 1=Y, 2=Z).
    static int handleAxisIndex(TransformHandle h);
    // Returns the submesh index of the currently active handle, or -1.
    int activeSubmeshIndex() const;

    struct Ray { glm::vec3 origin; glm::vec3 dir; };
    Ray rayFromCursor(scene::Scene* scene, int x, int y) const;
    glm::vec3 closestPointOnAxis(glm::vec3 axisOrigin, glm::vec3 axisDir,
                                  glm::vec3 rayOrigin, glm::vec3 rayDir) const;
    glm::vec3 intersectPlane(scene::Scene* scene, glm::vec3 planeNormal,
                              glm::vec3 planePoint, int x, int y) const;

    GizmoType resolveGizmoType() const;
    void loadGizmo(GizmoType type);
    void unloadGizmo();

    // Ensures the shared transform gizmo asset is loaded. The actual load is
    // deferred to a safe scene-update point to avoid creating GPU buffers
    // mid-render. Loading failures are unrecoverable (see implementation).
    void ensureTransformGizmoLoaded();

    // Static mesh cache: shared across all GizmoComponent instances
    static std::unordered_map<GizmoType, std::shared_ptr<scene::Drawable>> _gizmoCache;
    static std::shared_ptr<scene::Drawable> getCachedGizmo(GizmoType type, render::Engine* engine);

    static std::unordered_map<GizmoType, float> _gizmoScales;
    static std::unordered_map<GizmoType, float> _gizmoOpacities;
    static std::unordered_map<GizmoType, bool>  _gizmoVisible;
    static std::unordered_map<GizmoType, float> _gizmoScaleSensitivity;

    // Tracks the single node whose transform gizmo is currently visible.
    static std::weak_ptr<GizmoComponent> _currentTransform;

    // Scale sub-control visibility flags (shared across all instances).
    static bool _scaleUniformVisible;
    static bool _scaleAxisVisible;

    static bool _cleanupRegistered;
};

}
}
