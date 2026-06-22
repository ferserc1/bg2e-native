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

#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/MessageBox.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>

#include <cmath>

namespace bg2e::manipulation {

std::unordered_map<GizmoType, std::shared_ptr<scene::Drawable>> GizmoComponent::_gizmoCache;
bool GizmoComponent::_cleanupRegistered = false;

std::weak_ptr<GizmoComponent> GizmoComponent::_currentTransform;
bool GizmoComponent::_scaleUniformVisible = true;
bool GizmoComponent::_scaleAxisVisible = true;

std::unordered_map<GizmoType, float> GizmoComponent::_gizmoScales = {
    { GizmoType::Camera,           0.08f },
    { GizmoType::PointLight,       0.08f },
    { GizmoType::SpotLight,        0.08f },
    { GizmoType::DirectionalLight, 0.08f },
    { GizmoType::Environment,      0.08f },
    { GizmoType::Transform,        0.25f },
};

std::unordered_map<GizmoType, float> GizmoComponent::_gizmoOpacities = {
    { GizmoType::Camera,           0.6f },
    { GizmoType::PointLight,       0.6f },
    { GizmoType::SpotLight,        0.6f },
    { GizmoType::DirectionalLight, 0.6f },
    { GizmoType::Environment,      0.6f },
    { GizmoType::Transform,        1.0f },
};

std::unordered_map<GizmoType, bool> GizmoComponent::_gizmoVisible = {
    { GizmoType::Camera,           true },
    { GizmoType::PointLight,       true },
    { GizmoType::SpotLight,        true },
    { GizmoType::DirectionalLight, true },
    { GizmoType::Environment,      true },
    { GizmoType::Transform,        true },
};

GizmoComponent::GizmoComponent(render::Engine* engine)
    : _engine { engine }
{
    if (!_cleanupRegistered) {
        _cleanupRegistered = true;
        engine->cleanupManager().pushStatic([](VkDevice) {
            GizmoComponent::cleanupStatic();
        });
    }
}

GizmoType GizmoComponent::resolveGizmoType() const
{
    auto node = ownerNode();
    if (!node) return GizmoType::None;

    // Priority 1: Camera
    if (node->camera()) return GizmoType::Camera;

    // Priority 2: Light (sub-typed)
    auto lightComp = node->light();
    if (lightComp) {
        switch (lightComp->light().type()) {
            case base::Light::TypeOmni:       return GizmoType::PointLight;
            case base::Light::TypeSpot:        return GizmoType::SpotLight;
            case base::Light::TypeDirectional: return GizmoType::DirectionalLight;
            default: break;
        }
    }

    // Priority 3: Environment
    if (node->environment()) return GizmoType::Environment;

    return GizmoType::None;
}

std::shared_ptr<scene::Drawable> GizmoComponent::getCachedGizmo(GizmoType type, render::Engine* engine)
{
    auto it = _gizmoCache.find(type);
    if (it != _gizmoCache.end()) return it->second;

    std::shared_ptr<geo::Mesh> mesh;
    auto assetsPath = base::PlatformTools::assetPath();

    // The transform gizmo is loaded from a multi-submesh asset and needs
    // per-axis unlit materials assigned by submesh name. It is handled here
    // separately from the simple single-mesh type gizmos below.
    if (type == GizmoType::Transform) {
        std::shared_ptr<scene::Drawable> drawable;
        try {
            drawable = db::loadDrawableBg2(assetsPath, "transformGizmo.bg2", engine);
        }
        catch (const std::exception& e) {
            // Loading the transform gizmo asset is an unrecoverable error: show
            // it to the user and re-throw so the program aborts cleanly.
            app::MessageBox::showError(
                "Fatal error",
                std::string("Could not load the transform gizmo asset "
                            "'transformGizmo.bg2'. This is an unrecoverable error.\n\n")
                    + e.what()
            );
            throw;
        }

        // Assign unlit, axis-colored materials by submesh name. Naming maps:
        //   *A -> X (red), *C -> Y (green), *B -> Z (blue); scale* stays white.
        auto count = drawable->submeshesCount();
        for (uint32_t i = 0; i < count; ++i) {
            const auto name = drawable->submeshName(i);
            const auto group = drawable->submeshGroupName(i);
            auto has = [&](const char* token) {
                return name.find(token) != std::string::npos ||
                       group.find(token) != std::string::npos;
            };

            base::Color color = base::Color::White();
            if (has("translateA") || has("rotateA"))      color = base::Color::Red();    // X
            else if (has("translateC") || has("rotateC")) color = base::Color::Green();  // Y
            else if (has("translateB") || has("rotateB")) color = base::Color::Blue();   // Z
            // scaleUniform / scaleA / scaleB / scaleC stay white

            drawable->material(i).setIsUnlit(true);
            drawable->material(i).setAlbedo(color);
        }
        drawable->setRayTracingEnabled(false);
        drawable->updateMaterials();

        _gizmoCache[type] = drawable;
        return drawable;
    }

    switch (type) {
        case GizmoType::DirectionalLight: {
            auto meshData = db::loadMeshBg2(assetsPath, "dir-light-gizmo.bg2");
            mesh = meshData->mesh;
            break;
        }
        case GizmoType::SpotLight: {
            auto meshData = db::loadMeshBg2(assetsPath, "spot-light-gizmo.bg2");
            mesh = meshData->mesh;
            break;
        }
        case GizmoType::PointLight: {
            mesh = std::shared_ptr<geo::Mesh>(geo::createSphere(0.5f, 10, 10));
            break;
        }
        case GizmoType::Camera: {
            auto meshData = db::loadMeshBg2(assetsPath, "camera.bg2");
            mesh = meshData->mesh;
            break;
        }
        case GizmoType::Environment: {
            auto meshData = db::loadMeshBg2(assetsPath, "environment.bg2");
            mesh = meshData->mesh;
            break;
        }
        default:
            return nullptr;
    }

    auto drawable = std::make_shared<scene::Drawable>();
    drawable->setMesh(mesh);
    drawable->load(engine);
    drawable->material(0).setIsUnlit(true);
    drawable->setRayTracingEnabled(false);
    drawable->updateMaterials();

    _gizmoCache[type] = drawable;
    return drawable;
}

void GizmoComponent::loadGizmo(GizmoType type)
{
    auto cachedDrawable = getCachedGizmo(type, _engine);
    if (cachedDrawable) {
        setDrawable(cachedDrawable);
    }
}

void GizmoComponent::unloadGizmo()
{
    setDrawable(nullptr);
}

void GizmoComponent::ensureTransformGizmoLoaded()
{
    if (_gizmoCache.find(GizmoType::Transform) != _gizmoCache.end()) return;

    // Defer the GPU buffer creation to a safe scene-update point.
    auto engine = _engine;
    app::MainLoop::current()->safeUpdateScene([engine]() {
        getCachedGizmo(GizmoType::Transform, engine);
    });
}

std::shared_ptr<scene::Drawable> GizmoComponent::transformDrawable() const
{
    auto it = _gizmoCache.find(GizmoType::Transform);
    return it != _gizmoCache.end() ? it->second : nullptr;
}

void GizmoComponent::setCurrentTransform(scene::Node* node)
{
    // Hide the previously visible transform gizmo and reset the tracked one.
    if (auto prev = _currentTransform.lock()) {
        prev->setTransformVisible(false);
    }
    _currentTransform.reset();

    if (!node) return;

    if (auto giz = node->getComponent<GizmoComponent>()) {
        _currentTransform = std::static_pointer_cast<GizmoComponent>(giz->shared_from_this());
        giz->setTransformVisible(true);
    }
}

scene::Node* GizmoComponent::currentTransformNode()
{
    if (auto giz = _currentTransform.lock()) {
        return giz->ownerNode();
    }
    return nullptr;
}

bool GizmoComponent::isScaleUniformVisible() { return _scaleUniformVisible; }
void GizmoComponent::setScaleUniformVisible(bool visible) { _scaleUniformVisible = visible; }
bool GizmoComponent::isScaleAxisVisible() { return _scaleAxisVisible; }
void GizmoComponent::setScaleAxisVisible(bool visible) { _scaleAxisVisible = visible; }

bool GizmoComponent::isTransformSubmeshVisible(const std::string& name, const std::string& groupName)
{
    auto has = [&](const char* token) {
        return name.find(token) != std::string::npos ||
               groupName.find(token) != std::string::npos;
    };

    if (has("scaleUniform")) return _scaleUniformVisible;
    if (has("scaleA") || has("scaleB") || has("scaleC")) return _scaleAxisVisible;
    return true; // translate* and rotate* are always visible
}

TransformHandle GizmoComponent::handleForSubmesh(const std::string& name, const std::string& groupName)
{
    auto has = [&](const char* token) {
        return name.find(token) != std::string::npos ||
               groupName.find(token) != std::string::npos;
    };

    // Naming convention: A=X, B=Z, C=Y.
    if (has("scaleUniform")) return TransformHandle::ScaleUniform;
    if (has("translateA")) return TransformHandle::TranslateX;
    if (has("translateB")) return TransformHandle::TranslateZ;
    if (has("translateC")) return TransformHandle::TranslateY;
    if (has("rotateA"))    return TransformHandle::RotateX;
    if (has("rotateB"))    return TransformHandle::RotateZ;
    if (has("rotateC"))    return TransformHandle::RotateY;
    if (has("scaleA"))     return TransformHandle::ScaleX;
    if (has("scaleB"))     return TransformHandle::ScaleZ;
    if (has("scaleC"))     return TransformHandle::ScaleY;
    return TransformHandle::None;
}

uint32_t GizmoComponent::transformSubmeshIdentifier(uint32_t submeshIndex)
{
    auto drw = transformDrawable();
    if (!drw)
    {
        return 0;
    }

    auto count = drw->submeshesCount();
    if (submeshIndex >= count)
    {
        return 0;
    }

    if (_transformSubmeshIds.size() != count)
    {
        _transformSubmeshIds.assign(count, 0);
    }
    if (_transformSubmeshIds[submeshIndex] == 0)
    {
        _transformSubmeshIds[submeshIndex] = SelectableComponent::generateIdentifier();
    }
    return _transformSubmeshIds[submeshIndex];
}

void GizmoComponent::beginTransform(TransformHandle handle)
{
    _activeHandle = handle;
}

void GizmoComponent::endTransform()
{
    _activeHandle = TransformHandle::None;
}

void GizmoComponent::update(float)
{
    // Make sure the shared transform asset is available once this node becomes
    // the current transform target.
    if (_transformVisible) {
        ensureTransformGizmoLoaded();
    }

    auto newType = resolveGizmoType();
    if (newType == _currentGizmoType) return;

    _currentGizmoType = newType;

    // Defer the actual mesh swap to avoid destroying buffers mid-render
    app::MainLoop::current()->safeUpdateScene([this, newType]() {
        if (newType == GizmoType::None) {
            unloadGizmo();
        } else {
            loadGizmo(newType);
        }
    });
}

base::Color GizmoComponent::gizmoColor() const
{
    auto node = ownerNode();
    if (node)
    {
        auto lightComp = node->light();
        if (lightComp)
        {
            return lightComp->light().color();
        }
    }
    return base::Color::White();
}

glm::mat4 GizmoComponent::renderTransform(
    const glm::mat4& worldTransform,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix
) const {
    return renderTransform(worldTransform, viewMatrix, projMatrix, _currentGizmoType);
}

glm::mat4 GizmoComponent::renderTransform(
    const glm::mat4& worldTransform,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    GizmoType type
) const {
    glm::vec3 worldPosition = glm::vec3(worldTransform[3]);
    glm::vec4 viewPos = viewMatrix * glm::vec4(worldPosition, 1.0f);
    float viewDepth = std::abs(viewPos.z);
    float scale = gizmoScale(type) * viewDepth / projMatrix[1][1];

    glm::mat4 result = worldTransform;
    result[0] = glm::normalize(glm::vec4(glm::vec3(worldTransform[0]), 0.0f)) * scale;
    result[1] = glm::normalize(glm::vec4(glm::vec3(worldTransform[1]), 0.0f)) * scale;
    result[2] = glm::normalize(glm::vec4(glm::vec3(worldTransform[2]), 0.0f)) * scale;
    result[3] = worldTransform[3];
    return result;
}

float GizmoComponent::gizmoScale(GizmoType type)
{
    auto it = _gizmoScales.find(type);
    if (it != _gizmoScales.end()) return it->second;
    return 0.08f;
}

void GizmoComponent::setGizmoScale(GizmoType type, float scale)
{
    _gizmoScales[type] = scale;
}

float GizmoComponent::gizmoOpacity(GizmoType type)
{
    auto it = _gizmoOpacities.find(type);
    if (it != _gizmoOpacities.end()) return it->second;
    return 0.8f;
}

void GizmoComponent::setGizmoOpacity(GizmoType type, float opacity)
{
    _gizmoOpacities[type] = opacity;
}

bool GizmoComponent::isGizmoVisible(GizmoType type)
{
    auto it = _gizmoVisible.find(type);
    if (it != _gizmoVisible.end()) return it->second;
    return true;
}

void GizmoComponent::setGizmoVisible(GizmoType type, bool visible)
{
    _gizmoVisible[type] = visible;
}

void GizmoComponent::cleanupStatic()
{
    _gizmoCache.clear();
    _currentTransform.reset();
    _cleanupRegistered = false;
}

}
