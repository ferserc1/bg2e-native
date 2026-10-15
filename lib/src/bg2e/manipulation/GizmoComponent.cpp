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
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/TransformComponent.hpp>
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
#include <stdexcept>

namespace bg2e::manipulation {

std::unordered_map<GizmoType, std::shared_ptr<scene::Drawable>> GizmoComponent::_gizmoCache;
bool GizmoComponent::_cleanupRegistered = false;

std::weak_ptr<GizmoComponent> GizmoComponent::_currentTransform;
bool GizmoComponent::_scaleUniformVisible = true;
bool GizmoComponent::_scaleAxisVisible = true;

std::unordered_map<GizmoType, float> GizmoComponent::_gizmoScales = {
    {GizmoType::Camera, 0.08f},
    {GizmoType::PointLight, 0.08f},
    {GizmoType::SpotLight, 0.08f},
    {GizmoType::DirectionalLight, 0.08f},
    {GizmoType::Environment, 0.08f},
    {GizmoType::Transform, 0.25f},
};

std::unordered_map<GizmoType, float> GizmoComponent::_gizmoOpacities = {
    {GizmoType::Camera, 0.6f},
    {GizmoType::PointLight, 0.6f},
    {GizmoType::SpotLight, 0.6f},
    {GizmoType::DirectionalLight, 0.6f},
    {GizmoType::Environment, 0.6f},
    {GizmoType::Transform, 1.0f},
};

std::unordered_map<GizmoType, bool> GizmoComponent::_gizmoVisible = {
    {GizmoType::Camera, true},
    {GizmoType::PointLight, true},
    {GizmoType::SpotLight, true},
    {GizmoType::DirectionalLight, true},
    {GizmoType::Environment, true},
    {GizmoType::Transform, true},
};

std::unordered_map<GizmoType, float> GizmoComponent::_gizmoScaleSensitivity = {
    {GizmoType::Transform, 100.0f},
};

GizmoComponent::GizmoComponent(render::Engine* engine)
    : _engine{engine}, _mode{ToolExecutionMode::Production}
{
    if (!engine)
    {
        throw std::invalid_argument("GizmoComponent requires a production engine");
    }
    if (!_cleanupRegistered)
    {
        _cleanupRegistered = true;
        engine->cleanupManager().pushStatic([](VkDevice)
                                            {
                                                GizmoComponent::cleanupStatic();
                                            });
    }
}

GizmoComponent::GizmoComponent(draw::Engine* engine)
    : _drawEngine{engine}, _mode{ToolExecutionMode::Draw}
{
    if (!engine)
    {
        throw std::invalid_argument("GizmoComponent requires a draw engine");
    }
    setIgnoreSerialization(true);
}

std::shared_ptr<scene::Component> GizmoComponent::clone() const
{
    // A gizmo's state is entirely transient (current gizmo type, active transform
    // handle, drag-start data) and is rebuilt from the owner node on update(). The
    // clone is therefore a brand new gizmo bound to the same engine, not a copy of
    // the source's interaction state.
    auto result = _mode == ToolExecutionMode::Draw
        ? std::make_shared<GizmoComponent>(_drawEngine)
        : std::make_shared<GizmoComponent>(_engine);
    result->setPriority(priority());
    result->setIgnoreSerialization(ignoreSerialization());
    return result;
}

void GizmoComponent::didAddToNode(scene::Node* node)
{
    _ownerReference = node->weak_from_this();
}

void GizmoComponent::didRemoveFromNode(scene::Node*)
{
    endTransform();
    _ownerReference.reset();
    _transformVisible = false;
    if (_currentTransform.lock().get() == this)
    {
        _currentTransform.reset();
    }
}

GizmoType GizmoComponent::resolveGizmoType() const
{
    auto node = _ownerReference.lock();
    if (!node)
    {
        return GizmoType::None;
    }

    // Priority 1: Camera
    if (node->camera())
    {
        return GizmoType::Camera;
    }

    // Priority 2: Light (sub-typed)
    auto lightComp = node->light();
    if (lightComp)
    {
        switch (lightComp->light().type())
        {
        case base::Light::TypeOmni:
            return GizmoType::PointLight;
        case base::Light::TypeSpot:
            return GizmoType::SpotLight;
        case base::Light::TypeDirectional:
            return GizmoType::DirectionalLight;
        default:
            break;
        }
    }

    // Priority 3: Environment
    if (node->environment())
    {
        return GizmoType::Environment;
    }

    return GizmoType::None;
}

std::shared_ptr<scene::Drawable> GizmoComponent::getCachedGizmo(GizmoType type, render::Engine* engine)
{
    auto it = _gizmoCache.find(type);
    if (it != _gizmoCache.end())
    {
        return it->second;
    }

    std::shared_ptr<geo::Mesh> mesh;
    auto assetsPath = base::PlatformTools::assetPath();

    // The transform gizmo is loaded from a multi-submesh asset and needs
    // per-axis unlit materials assigned by submesh name. It is handled here
    // separately from the simple single-mesh type gizmos below.
    if (type == GizmoType::Transform)
    {
        std::shared_ptr<scene::Drawable> drawable;
        try
        {
            drawable = db::loadDrawableBg2(assetsPath, "transformGizmo.bg2", engine);
        }
        catch (const std::exception& e)
        {
            // Loading the transform gizmo asset is an unrecoverable error: show
            // it to the user and re-throw so the program aborts cleanly.
            app::MessageBox::showError(
                "Fatal error",
                std::string("Could not load the transform gizmo asset "
                            "'transformGizmo.bg2'. This is an unrecoverable error.\n\n") +
                    e.what());
            throw;
        }

        // Assign unlit, axis-colored materials by submesh name. Naming maps:
        //   *A -> X (red), *C -> Y (green), *B -> Z (blue); scale* stays white.
        auto count = drawable->submeshesCount();
        for (uint32_t i = 0; i < count; ++i)
        {
            const auto name = drawable->submeshName(i);
            const auto group = drawable->submeshGroupName(i);
            auto has = [&](const char* token)
            {
                return name.find(token) != std::string::npos ||
                       group.find(token) != std::string::npos;
            };

            base::Color color = base::Color::White();
            if (has("translateA") || has("rotateA"))
            {
                color = base::Color::Red(); // X
            }
            else if (has("translateC") || has("rotateC"))
            {
                color = base::Color::Green(); // Y
            }
            else if (has("translateB") || has("rotateB"))
            {
                color = base::Color::Blue(); // Z
            }
            // scaleUniform / scaleA / scaleB / scaleC stay white

            drawable->material(i).setIsUnlit(true);
            drawable->material(i).setAlbedo(color);
        }
        drawable->setRayTracingEnabled(false);
        drawable->updateMaterials();

        _gizmoCache[type] = drawable;
        return drawable;
    }

    switch (type)
    {
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
    if (_mode == ToolExecutionMode::Draw)
    {
        if (auto resource = getDrawGizmo(type))
        {
            setDrawable(resource);
        }
        return;
    }
    auto cachedDrawable = getCachedGizmo(type, _engine);
    if (cachedDrawable)
    {
        setDrawable(cachedDrawable);
    }
}

void GizmoComponent::unloadGizmo()
{
    setDrawable(nullptr);
}

void GizmoComponent::ensureTransformGizmoLoaded()
{
    if (_mode == ToolExecutionMode::Draw)
    {
        getDrawGizmo(GizmoType::Transform);
    }
    else
    {
        getCachedGizmo(GizmoType::Transform, _engine);
    }
}

std::shared_ptr<scene::Drawable> GizmoComponent::transformDrawable() const
{
    if (_mode != ToolExecutionMode::Production)
    {
        throw std::logic_error("Use transformDrawDrawable for a draw gizmo");
    }
    auto it = _gizmoCache.find(GizmoType::Transform);
    return it != _gizmoCache.end() ? it->second : nullptr;
}

std::shared_ptr<draw::Drawable> GizmoComponent::transformDrawDrawable() const
{
    if (_mode != ToolExecutionMode::Draw)
    {
        throw std::logic_error("Use transformDrawable for a production gizmo");
    }
    auto it = _drawGizmoCache.find(GizmoType::Transform);
    return it != _drawGizmoCache.end() ? it->second : nullptr;
}

std::shared_ptr<draw::Drawable> GizmoComponent::getDrawGizmo(GizmoType type)
{
    if (auto it = _drawGizmoCache.find(type); it != _drawGizmoCache.end())
    {
        return it->second;
    }
    auto assets = base::PlatformTools::assetPath();
    std::shared_ptr<geo::MeshPNUUT> mesh;
    std::unique_ptr<db::Bg2Mesh> parsed;
    switch (type)
    {
    case GizmoType::Transform:
        parsed.reset(db::loadMeshBg2(assets, "transformGizmo.bg2"));
        break;
    case GizmoType::DirectionalLight:
        parsed.reset(db::loadMeshBg2(assets, "dir-light-gizmo.bg2"));
        break;
    case GizmoType::SpotLight:
        parsed.reset(db::loadMeshBg2(assets, "spot-light-gizmo.bg2"));
        break;
    case GizmoType::Camera:
        parsed.reset(db::loadMeshBg2(assets, "camera.bg2"));
        break;
    case GizmoType::Environment:
        parsed.reset(db::loadMeshBg2(assets, "environment.bg2"));
        break;
    case GizmoType::PointLight:
        mesh.reset(geo::createSphere(0.5f, 10, 10));
        break;
    default:
        return nullptr;
    }
    if (parsed)
    {
        mesh = parsed->mesh;
    }
    auto resource = std::make_shared<draw::Drawable>();
    resource->setMesh(mesh);
    resource->setRayTracingEnabled(false);
    for (uint32_t i = 0; i < resource->submeshesCount(); ++i)
    {
        if (parsed && i < parsed->materials.size())
        {
            resource->setMaterial(parsed->materials[i], i);
        }
        // The BG2 reader retains submesh names/groups in material metadata.
        const auto& material = resource->material(i);
        resource->setSubmeshName(material.name(), i);
        resource->setSubmeshGroupName(material.groupName(), i);
        auto handle = handleForSubmesh(resource->submeshName(i), resource->submeshGroupName(i));
        base::Color color = base::Color::White();
        if (handle == TransformHandle::TranslateX || handle == TransformHandle::RotateX)
        {
            color = base::Color::Red();
        }
        else if (handle == TransformHandle::TranslateY || handle == TransformHandle::RotateY)
        {
            color = base::Color::Green();
        }
        else if (handle == TransformHandle::TranslateZ || handle == TransformHandle::RotateZ)
        {
            color = base::Color::Blue();
        }
        resource->material(i).setIsUnlit(true);
        resource->material(i).setAlbedo(color);
    }
    resource->load(_drawEngine);
    _drawGizmoCache[type] = resource;
    return resource;
}

void GizmoComponent::cleanupDrawResources()
{
    if (_mode != ToolExecutionMode::Draw)
    {
        throw std::logic_error("cleanupDrawResources requires a draw gizmo");
    }
    endTransform();
    clearDrawable();
    _drawGizmoCache.clear();
    _transformSubmeshIds.clear();
    _currentGizmoType = GizmoType::None;
    _transformVisible = false;
    if (_currentTransform.lock().get() == this)
    {
        _currentTransform.reset();
    }
}

void GizmoComponent::synchronizeResources()
{
    const auto type = resolveGizmoType();
    if (type != _currentGizmoType)
    {
        if (type == GizmoType::None)
        {
            unloadGizmo();
        }
        else
        {
            loadGizmo(type);
        }
        _currentGizmoType = type;
    }
    if (_transformVisible)
    {
        ensureTransformGizmoLoaded();
    }
}

void GizmoComponent::setCurrentTransform(scene::Node* node)
{
    // Hide the previously visible transform gizmo and reset the tracked one.
    if (auto prev = _currentTransform.lock())
    {
        prev->setTransformVisible(false);
    }
    _currentTransform.reset();

    if (!node)
    {
        return;
    }

    if (auto giz = node->getComponent<GizmoComponent>())
    {
        _currentTransform = std::static_pointer_cast<GizmoComponent>(giz->shared_from_this());
        giz->setTransformVisible(true);
    }
}

scene::Node* GizmoComponent::currentTransformNode()
{
    if (auto giz = _currentTransform.lock())
    {
        return giz->_ownerReference.lock().get();
    }
    return nullptr;
}

bool GizmoComponent::isScaleUniformVisible()
{
    return _scaleUniformVisible;
}
void GizmoComponent::setScaleUniformVisible(bool visible)
{
    _scaleUniformVisible = visible;
}
bool GizmoComponent::isScaleAxisVisible()
{
    return _scaleAxisVisible;
}
void GizmoComponent::setScaleAxisVisible(bool visible)
{
    _scaleAxisVisible = visible;
}

bool GizmoComponent::isTransformSubmeshVisible(const std::string& name, const std::string& groupName)
{
    auto has = [&](const char* token)
    {
        return name.find(token) != std::string::npos ||
               groupName.find(token) != std::string::npos;
    };

    if (has("scaleUniform"))
    {
        return _scaleUniformVisible;
    }
    if (has("scaleA") || has("scaleB") || has("scaleC"))
    {
        return _scaleAxisVisible;
    }
    return true; // translate* and rotate* are always visible
}

TransformHandle GizmoComponent::handleForSubmesh(const std::string& name, const std::string& groupName)
{
    auto has = [&](const char* token)
    {
        return name.find(token) != std::string::npos ||
               groupName.find(token) != std::string::npos;
    };

    // Naming convention: A=X, B=Z, C=Y.
    if (has("scaleUniform"))
    {
        return TransformHandle::ScaleUniform;
    }
    if (has("translateA"))
    {
        return TransformHandle::TranslateX;
    }
    if (has("translateB"))
    {
        return TransformHandle::TranslateZ;
    }
    if (has("translateC"))
    {
        return TransformHandle::TranslateY;
    }
    if (has("rotateA"))
    {
        return TransformHandle::RotateX;
    }
    if (has("rotateB"))
    {
        return TransformHandle::RotateZ;
    }
    if (has("rotateC"))
    {
        return TransformHandle::RotateY;
    }
    if (has("scaleA"))
    {
        return TransformHandle::ScaleX;
    }
    if (has("scaleB"))
    {
        return TransformHandle::ScaleZ;
    }
    if (has("scaleC"))
    {
        return TransformHandle::ScaleY;
    }
    return TransformHandle::None;
}

uint32_t GizmoComponent::transformSubmeshIdentifier(uint32_t submeshIndex)
{
    auto drawResource = _mode == ToolExecutionMode::Draw ? transformDrawDrawable() : nullptr;
    auto resource = _mode == ToolExecutionMode::Production ? transformDrawable() : nullptr;
    auto count = drawResource ? drawResource->submeshesCount() : resource ? resource->submeshesCount() : 0;
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

void GizmoComponent::beginTransform(TransformHandle handle, int mouseX, int mouseY)
{
    _activeHandle = handle;
    _mouseDownX = mouseX;
    _mouseDownY = mouseY;

    auto node = _ownerReference.lock();
    if (!node || !node->transform())
    {
        return;
    }

    auto transform = node->transform();
    _initialLocalMatrix = transform->matrix();
    _localRotation = scene::TransformComponent::extractRotation(_initialLocalMatrix);
    _initialScale = scene::TransformComponent::extractScale(_initialLocalMatrix);
    _initialWorldPos = glm::vec3(node->worldMatrix()[3]);

    int axis = handleAxisIndex(handle);
    if (axis >= 0)
    {
        _localAxis = glm::normalize(glm::vec3(_localRotation[axis]));

        auto parent = node->parent();
        if (parent)
        {
            auto parentWM = parent->worldMatrix();
            _parentRotation = scene::TransformComponent::extractRotation(parentWM);
            auto parentScale = scene::TransformComponent::extractScale(parentWM);
            _parentScaleAxis = parentScale[axis];
        }
        else
        {
            _parentRotation = glm::mat3(1.0f);
            _parentScaleAxis = 1.0f;
        }
        _worldAxis = glm::normalize(_parentRotation * _localAxis);
    }

    // For translation: capture the initial click point projected onto the axis
    // so we can compute deltas relative to it (avoiding the origin jump)
    if (handle >= TransformHandle::TranslateX && handle <= TransformHandle::TranslateZ)
    {
        auto node = _ownerReference.lock();
        auto scene = node->scene();
        auto ray = rayFromCursor(scene, mouseX, mouseY);
        _initialAxisPoint = closestPointOnAxis(
            _initialWorldPos, _worldAxis, ray.origin, ray.dir);
    }

    // For rotation: compute initial intersection with the perpendicular plane
    if (handle >= TransformHandle::RotateX && handle <= TransformHandle::RotateZ)
    {
        auto node = _ownerReference.lock();
        auto scene = node->scene();
        _initialIntersection = intersectPlane(scene, _worldAxis, _initialWorldPos, mouseX, mouseY);
    }

    // Highlight active handle: store original color, set yellow
    auto drawResource = _mode == ToolExecutionMode::Draw ? transformDrawDrawable() : nullptr;
    auto drw = _mode == ToolExecutionMode::Production ? transformDrawable() : nullptr;
    int submeshIdx = activeSubmeshIndex();
    if (drawResource && submeshIdx >= 0)
    {
        _originalHandleColor = drawResource->material(submeshIdx).albedo();
        drawResource->material(submeshIdx).setAlbedo(base::Color::Yellow());
    }
    if (drw && submeshIdx >= 0)
    {
        _originalHandleColor = drw->renderMaterial(submeshIdx)->materialAttributes().albedo();
        drw->material(submeshIdx).setAlbedo(base::Color::Yellow());
        drw->updateMaterials();
    }
}

void GizmoComponent::updateTransform(scene::Scene* scene, int mouseX, int mouseY)
{
    if (_activeHandle == TransformHandle::None)
    {
        return;
    }

    auto node = _ownerReference.lock();
    if (!node || !node->transform())
    {
        return;
    }

    switch (_activeHandle)
    {
    case TransformHandle::TranslateX:
    case TransformHandle::TranslateY:
    case TransformHandle::TranslateZ: {
        auto ray = rayFromCursor(scene, mouseX, mouseY);
        auto closest = closestPointOnAxis(
            _initialWorldPos, _worldAxis, ray.origin, ray.dir);
        float dist = glm::dot(closest - _initialAxisPoint, _worldAxis);
        glm::vec3 deltaLocal = (_localAxis * dist) / _parentScaleAxis;
        glm::vec3 t = scene::TransformComponent::extractTranslation(_initialLocalMatrix) + deltaLocal;
        node->transform()->setMatrix(
            scene::TransformComponent::recompose(_localRotation, _initialScale, t));
        break;
    }

    case TransformHandle::RotateX:
    case TransformHandle::RotateY:
    case TransformHandle::RotateZ: {
        auto currentIntersection = intersectPlane(scene, _worldAxis, _initialWorldPos, mouseX, mouseY);
        glm::vec3 v0 = _initialIntersection - _initialWorldPos;
        glm::vec3 v1 = currentIntersection - _initialWorldPos;
        float len0 = glm::length(v0);
        float len1 = glm::length(v1);
        if (len0 < 1e-6f || len1 < 1e-6f)
        {
            break;
        }
        v0 /= len0;
        v1 /= len1;
        float angle = std::atan2(
            glm::dot(_worldAxis, glm::cross(v0, v1)),
            glm::dot(v0, v1));
        glm::mat3 R_delta = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, _localAxis));
        auto t = scene::TransformComponent::extractTranslation(_initialLocalMatrix);
        auto newRotation = R_delta * _localRotation;
        node->transform()->setMatrix(
            scene::TransformComponent::recompose(newRotation, _initialScale, t));
        break;
    }

    case TransformHandle::ScaleX:
    case TransformHandle::ScaleY:
    case TransformHandle::ScaleZ: {
        float delta = ((mouseX - _mouseDownX) + (mouseY - _mouseDownY)) / 2.0f;
        float factor = std::exp(delta / gizmoScaleSensitivity(GizmoType::Transform));
        glm::vec3 sf = _initialScale;
        sf[handleAxisIndex(_activeHandle)] *= factor;
        auto t = scene::TransformComponent::extractTranslation(_initialLocalMatrix);
        node->transform()->setMatrix(
            scene::TransformComponent::recompose(_localRotation, sf, t));
        break;
    }

    case TransformHandle::ScaleUniform: {
        float delta = ((mouseX - _mouseDownX) + (mouseY - _mouseDownY)) / 2.0f;
        float factor = std::exp(delta / gizmoScaleSensitivity(GizmoType::Transform));
        glm::vec3 sf = _initialScale * factor;
        auto t = scene::TransformComponent::extractTranslation(_initialLocalMatrix);
        node->transform()->setMatrix(
            scene::TransformComponent::recompose(_localRotation, sf, t));
        break;
    }

    default:
        break;
    }
}

void GizmoComponent::endTransform()
{
    // Restore original handle color
    auto drawResource = _mode == ToolExecutionMode::Draw ? transformDrawDrawable() : nullptr;
    auto drw = _mode == ToolExecutionMode::Production ? transformDrawable() : nullptr;
    int submeshIdx = activeSubmeshIndex();
    if (drawResource && submeshIdx >= 0)
    {
        drawResource->material(submeshIdx).setAlbedo(_originalHandleColor);
    }
    if (drw && submeshIdx >= 0)
    {
        drw->material(submeshIdx).setAlbedo(_originalHandleColor);
        drw->updateMaterials();
    }
    _activeHandle = TransformHandle::None;
}

int GizmoComponent::handleAxisIndex(TransformHandle h)
{
    switch (h)
    {
    case TransformHandle::TranslateX:
    case TransformHandle::RotateX:
    case TransformHandle::ScaleX:
        return 0;
    case TransformHandle::TranslateY:
    case TransformHandle::RotateY:
    case TransformHandle::ScaleY:
        return 1;
    case TransformHandle::TranslateZ:
    case TransformHandle::RotateZ:
    case TransformHandle::ScaleZ:
        return 2;
    default:
        return -1;
    }
}

int GizmoComponent::activeSubmeshIndex() const
{
    if (_activeHandle == TransformHandle::None)
    {
        return -1;
    }
    auto drawResource = _mode == ToolExecutionMode::Draw ? transformDrawDrawable() : nullptr;
    auto drw = _mode == ToolExecutionMode::Production ? transformDrawable() : nullptr;
    auto count = drawResource ? drawResource->submeshesCount() : drw ? drw->submeshesCount() : 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (handleForSubmesh(drawResource ? drawResource->submeshName(i) : drw->submeshName(i),
            drawResource ? drawResource->submeshGroupName(i) : drw->submeshGroupName(i)) == _activeHandle)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

GizmoComponent::Ray GizmoComponent::rayFromCursor(scene::Scene* scene, int x, int y) const
{
    auto camera = scene->mainCamera();
    auto vp = camera->projection()->viewport();
    auto projMatrix = camera->projectionMatrix();
    auto viewMatrix = camera->ownerNode()->invertedWorldMatrix();

    float ndcX = (2.0f * static_cast<float>(x) / vp.width) - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(y) / vp.height);

    auto invVP = glm::inverse(projMatrix * viewMatrix);
    glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    Ray ray;
    ray.origin = glm::vec3(nearPoint);
    ray.dir = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
    return ray;
}

glm::vec3 GizmoComponent::closestPointOnAxis(
    glm::vec3 axisOrigin, glm::vec3 axisDir,
    glm::vec3 rayOrigin, glm::vec3 rayDir) const
{
    glm::vec3 w = axisOrigin - rayOrigin;
    float a = glm::dot(axisDir, axisDir);
    float b = glm::dot(axisDir, rayDir);
    float c = glm::dot(rayDir, rayDir);
    float d = glm::dot(axisDir, w);
    float e = glm::dot(rayDir, w);
    float denom = a * c - b * b;
    if (std::abs(denom) < 1e-6f)
    {
        return axisOrigin;
    }
    float t = (b * e - c * d) / denom;
    return axisOrigin + t * axisDir;
}

glm::vec3 GizmoComponent::intersectPlane(
    scene::Scene* scene, glm::vec3 planeNormal, glm::vec3 planePoint,
    int x, int y) const
{
    auto ray = rayFromCursor(scene, x, y);
    float denom = glm::dot(planeNormal, ray.dir);
    if (std::abs(denom) < 1e-6f)
    {
        return planePoint;
    }
    float t = glm::dot(planeNormal, planePoint - ray.origin) / denom;
    return ray.origin + t * ray.dir;
}

float GizmoComponent::gizmoScaleSensitivity(GizmoType type)
{
    auto it = _gizmoScaleSensitivity.find(type);
    if (it != _gizmoScaleSensitivity.end())
    {
        return it->second;
    }
    return 100.0f;
}

void GizmoComponent::setGizmoScaleSensitivity(GizmoType type, float sensitivity)
{
    _gizmoScaleSensitivity[type] = sensitivity;
}

void GizmoComponent::update(float)
{
    const bool needsTransform = _transformVisible &&
        (_mode == ToolExecutionMode::Draw ? !transformDrawDrawable() : !transformDrawable());
    if (_resourceUpdatePending || (resolveGizmoType() == _currentGizmoType && !needsTransform))
    {
        return;
    }
    auto loop = app::MainLoop::current();
    if (!loop)
    {
        throw std::logic_error("Gizmo resource changes require MainLoop or explicit synchronizeResources at a stopped boundary");
    }
    auto weak = weak_from_this();
    _resourceUpdatePending = true;
    loop->safeUpdateScene([weak]()
    {
        if (auto component = weak.lock())
        {
            auto gizmo = std::static_pointer_cast<GizmoComponent>(component);
            gizmo->_resourceUpdatePending = false;
            if (auto node = gizmo->_ownerReference.lock(); node && node->getComponent<GizmoComponent>() == gizmo.get())
            {
                gizmo->synchronizeResources();
            }
        }
    });
}

base::Color GizmoComponent::gizmoColor() const
{
    auto node = _ownerReference.lock();
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
    const glm::mat4& projMatrix) const
{
    return renderTransform(worldTransform, viewMatrix, projMatrix, _currentGizmoType);
}

glm::mat4 GizmoComponent::renderTransform(
    const glm::mat4& worldTransform,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    GizmoType type) const
{
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
    if (it != _gizmoScales.end())
    {
        return it->second;
    }
    return 0.08f;
}

void GizmoComponent::setGizmoScale(GizmoType type, float scale)
{
    _gizmoScales[type] = scale;
}

float GizmoComponent::gizmoOpacity(GizmoType type)
{
    auto it = _gizmoOpacities.find(type);
    if (it != _gizmoOpacities.end())
    {
        return it->second;
    }
    return 0.8f;
}

void GizmoComponent::setGizmoOpacity(GizmoType type, float opacity)
{
    _gizmoOpacities[type] = opacity;
}

bool GizmoComponent::isGizmoVisible(GizmoType type)
{
    auto it = _gizmoVisible.find(type);
    if (it != _gizmoVisible.end())
    {
        return it->second;
    }
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
