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

namespace bg2e::manipulation {

std::unordered_map<GizmoType, std::shared_ptr<scene::Drawable>> GizmoComponent::_gizmoCache;

GizmoComponent::GizmoComponent(render::Engine* engine)
    : _engine { engine }
{
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

void GizmoComponent::update(float)
{
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

void GizmoComponent::cleanupStatic()
{
    _gizmoCache.clear();
}

}
