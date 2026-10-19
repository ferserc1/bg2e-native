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

#include "StageScene.hpp"

#include <bg2e/base/Color.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/db/scene.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/db/scene_bg2.hpp>
#include <bg2e/db/scene_gltf.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/math/projections.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/BoundingBox.hpp>
#include <bg2e/utils/SceneCameraUtils.hpp>

#include <exception>
#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>
#include <algorithm>

namespace model_edit {
namespace {
class CameraProjection final : public bg2e::math::PerspectiveProjection {
  public:
    std::shared_ptr<bg2e::math::Projection> clone() const override
    {
        return std::make_shared<CameraProjection>(*this);
    }

    void apply(glm::mat4& matrix) override
    {
        matrix = glm::perspectiveRH_ZO(glm::radians(fov()), _viewport.width / _viewport.height, near(), far());
    }
};
}

StageScene::StageScene(bg2e::draw::Engine* engine) : _engine(engine)
{
    if (!_engine)
    {
        throw std::invalid_argument("The model editor stage requires a draw Engine");
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::create()
{
    using namespace bg2e;
    if (_root)
    {
        throw std::logic_error("The model editor stage is already initialized");
    }
    _root = std::make_shared<scene::Node>("Model editor stage");
    _root->addComponent(new scene::EnvironmentComponent());

    auto model = std::make_shared<draw::Drawable>();
    model->setMesh(geo::createCube(2, 2, 2));
    model->iterateMaterials([](auto& material)
    {
        material.setAlbedo(base::Color(0.25f, 0.55f, 0.8f));
        material.setMetalness(0);
        material.setRoughness(0.45f);
        material.setAlphaCutoff(0);
    });
    addDrawable("Startup model", model, {0, 0, 0});
    _model = _root->children().back();

    auto floor = std::make_shared<draw::Drawable>();
    floor->setMesh(geo::createPlane(20, 20));
    floor->setRayTracingEnabled(false); // Editor fixture, excluded from future bake snapshots.
    floor->material().setAlbedo(base::Color(0.4f, 0.4f, 0.4f));
    floor->material().setMetalness(0);
    floor->material().setRoughness(0.8f);
    floor->material().setAlphaCutoff(0);
    addDrawable("Floor", floor, {0, -1.05f, 0});
    _floor = _root->children().back();

    _camera = std::make_shared<scene::Node>("Editor camera");
    _camera->addComponent(new scene::TransformComponent());
    _camera->addComponent(new scene::OrbitCameraComponent());
    _camera->addComponent(new scene::CameraComponent());
    _camera->camera()->setProjection(new CameraProjection());
    auto* orbit = _camera->getComponent<scene::OrbitCameraComponent>();
    orbit->setWheelSpeed(0.4f);
    orbit->setRotateButtons(true, false, false);
    orbit->setPanButtons(false, false, true);
    orbit->setZoomButtons(false, true, false);
    resetCamera();
    _root->addChild(_camera);

    auto light = std::make_shared<scene::Node>("Stage light");
    light->addComponent(scene::TransformComponent::makeTranslated(2, 2, 2));
    light->addComponent(new scene::LightComponent());
    light->light()->light().setType(base::Light::TypeOmni);
    light->light()->light().setIntensity(45);
    light->light()->light().setCastShadows(true);
    _root->addChild(light);
    _fillLight = light;
    _scene = std::make_shared<scene::Scene>();
    _scene->setSceneRoot(_root);
    _scene->setMainCamera(_camera->camera());
    prepareTools(_fillLight);
    return _root;
}

void StageScene::addDrawable(const char* name, std::shared_ptr<bg2e::draw::Drawable> drawable,
    const glm::vec3& position)
{
    _drawables.push_back(drawable); // Retain partial loads until Engine-bound cleanup.
    drawable->load(_engine);
    auto node = std::make_shared<bg2e::scene::Node>(name);
    node->addComponent(bg2e::scene::TransformComponent::makeTranslated(position));
    node->addComponent(new bg2e::scene::DrawableComponent(std::move(drawable)));
    _root->addChild(node);
}

void StageScene::resetCamera()
{
    if (_camera)
    {
        auto* orbit = _camera->getComponent<bg2e::scene::OrbitCameraComponent>();
        orbit->setCenter({0, 0, 0});
        orbit->setDistance(8);
        orbit->setRotation({15.0f, 30.0f});
    }
}

bg2e::scene::OrbitCameraComponent* StageScene::orbitCamera() const
{
    return _camera ? _camera->getComponent<bg2e::scene::OrbitCameraComponent>() : nullptr;
}

bool StageScene::frameNode(bg2e::scene::Node* node)
{
    if (!node || !camera() || !orbitCamera())
    {
        return false;
    }
    const bg2e::scene::BoundingBox bounds(node);
    if (!bounds.isValid())
    {
        return false;
    }
    const float radius = std::max(glm::length(bounds.halfSize()), 0.01f);
    const auto viewport = camera()->projection()->viewport();
    const float aspect = viewport.width > 0 && viewport.height > 0 ?
        float(viewport.width) / float(viewport.height) : 1.0f;
    const float verticalFov = glm::radians(bg2e::utils::cameraVerticalFieldOfView(camera()));
    const float horizontalFov = 2.0f * std::atan(std::tan(verticalFov * 0.5f) * std::max(aspect, 0.001f));
    const float fov = std::min(verticalFov, horizontalFov);
    const float distance = radius / std::sin(fov * 0.5f) * 1.2f;
    if (!std::isfinite(distance) || distance <= 0.0f)
    {
        return false;
    }
    orbitCamera()->setCenter(bounds.center());
    orbitCamera()->setDistance(distance);
    return true;
}

std::vector<bg2e::utils::ViewSample> StageScene::analyzeBestView(bg2e::scene::Node* node)
{
    if (!node)
    {
        return {};
    }
    if (!_viewpointAnalyzer)
    {
        _viewpointAnalyzer = std::make_unique<bg2e::utils::ViewpointAnalyzer>(_engine);
    }
    auto config = _viewpointAnalyzer->config();
    const float fov = bg2e::utils::cameraVerticalFieldOfView(camera());
    const auto viewport = camera()->projection()->viewport();
    const float aspect = viewport.width > 0 && viewport.height > 0 ?
        float(viewport.width) / float(viewport.height) : 1.0f;
    if (aspect >= 1.0f)
    {
        config.width = 48;
        config.height = std::max(24u, static_cast<uint32_t>(std::lround(48.0f / aspect)));
    }
    else
    {
        config.width = std::max(24u, static_cast<uint32_t>(std::lround(48.0f * aspect)));
        config.height = 48;
    }
    config.targetHeight = {0.5f, 0.5f, 1, false};
    config.distanceScale = {1.15f, 1.15f, 1, false};
    config.fieldOfView = {fov, fov, 1, false};
    _viewpointAnalyzer->setConfig(config);
    return _viewpointAnalyzer->analyze(node);
}

void StageScene::applyViewpoint(const bg2e::utils::CameraCandidate& candidate)
{
    bg2e::utils::applyViewpointToOrbitCamera(candidate, orbitCamera(), camera());
}

const std::vector<bg2e::utils::AnalyzedSubmesh>& StageScene::analyzedSubmeshes() const
{
    static const std::vector<bg2e::utils::AnalyzedSubmesh> empty;
    return _viewpointAnalyzer ? _viewpointAnalyzer->analyzedSubmeshes() : empty;
}

namespace {
void cleanupTree(bg2e::scene::Node& node)
{
    std::exception_ptr error;
    auto release = [&](const auto& operation)
    {
        try
        {
            operation();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
    };
    if (auto gizmo = node.getComponent<bg2e::manipulation::GizmoComponent>())
    {
        release([&]
        {
            gizmo->cleanupDrawResources();
        });
    }
    if (auto component = node.getComponent<bg2e::scene::DrawableComponent>())
    {
        if (auto drawable = component->drawDrawable())
        {
            release([&]
            {
                drawable->cleanup();
            });
        }
    }
    for (const auto& child : node.children())
    {
        release([&]
        {
            cleanupTree(*child);
        });
    }
    if (error)
    {
        std::rethrow_exception(error);
    }
}
}

std::shared_ptr<bg2e::scene::Node> StageScene::load(const std::filesystem::path& input)
{
    using namespace bg2e;
    const auto path = std::filesystem::absolute(input).lexically_normal();
    const auto extension = path.extension().string();
    if (extension == ".bg2" || extension == ".vwglb")
    {
        return db::loadSceneBg2(path, *_engine);
    }
    if (extension == ".gltf" || extension == ".glb")
    {
        return db::loadGltf(path, *_engine);
    }
    if (extension == ".obj")
    {
        auto drawable = db::loadDrawableObj(path, *_engine);
        auto node = std::make_shared<scene::Node>(path.stem().string());
        auto component = std::make_shared<scene::DrawableComponent>(drawable);
        component->setResourcePath(path);
        node->addComponent(component);
        node->addComponent(new scene::TransformComponent());
        return node;
    }
    if (extension == ".json" || (extension == ".bg2scene" || extension == ".bg2e"))
    {
        auto loaded = db::loadScene(path, *_engine);
        auto children = loaded->rootNode()->children();
        if (children.size() == 1)
        {
            loaded->rootNode()->removeChild(children[0]);
            return children[0];
        }
        auto result = std::make_shared<scene::Node>(path.stem().string());
        for (auto& child : children)
        {
            loaded->rootNode()->removeChild(child);
            result->addChild(child);
        }
        return result;
    }
    throw std::invalid_argument("Supported files: BG2, VWGLB, OBJ/MTL, glTF/GLB, JSON scene");
}

void StageScene::adopt(std::shared_ptr<bg2e::scene::Node> candidate, const std::filesystem::path& source)
{
    if (!candidate)
    {
        throw std::invalid_argument("Cannot adopt an empty model");
    }
    Document next;
    next.root = candidate;
    next.sourcePath = std::filesystem::absolute(source).lexically_normal();
    const auto extension = source.extension().string();
    next.dirty = extension != ".bg2" && extension != ".vwglb" && extension != ".json" && extension != ".bg2scene" && extension != ".bg2e";
    if (!next.dirty)
    {
        next.savePath = next.sourcePath;
    }
    next.generation = _document.generation + 1;
    // Reserve all allocating state before changing the stage graph.
    _retired.reserve(_retired.size() + 1);
    _root->addChild(candidate);
    if (_model)
    {
        _retired.push_back(_model);
        _root->removeChild(_model);
    }
    _model = std::move(candidate);
    _document = std::move(next);
    if (environmentNode() && _root->environment())
    {
        _root->removeComponent(_root->environment()->shared_from_this());
    }
    _scene->updateAll();
}

void StageScene::save(const std::filesystem::path& path)
{
    if (!_document.root)
    {
        throw std::logic_error("Open a document before saving");
    }
    auto committedPath = std::filesystem::absolute(path).lexically_normal();
    const auto extension = path.extension().string();
    if (extension == ".bg2" || extension == ".vwglb")
    {
        bg2e::db::storeDrawableBg2(path, *_engine, _document.root.get());
    }
    else if (extension == ".json" || (extension == ".bg2scene" || extension == ".bg2e"))
    {
        bg2e::db::saveScene(_document.root.get(), *_engine, path);
    }
    else
    {
        throw std::invalid_argument("Save as .json/.bg2scene to preserve the graph, or .bg2 for one model");
    }
    // Commit metadata after the resource transaction, including filesystem path allocation.
    _document.savePath = std::move(committedPath);
    _document.dirty = false;
}

void StageScene::releaseRetired()
{
    std::exception_ptr error;
    for (const auto& node : _retired)
    {
        try
        {
            cleanupTree(*node);
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
    }
    _retired.clear();
    if (error)
    {
        std::rethrow_exception(error);
    }
}


namespace {
void visitNodes(const std::shared_ptr<bg2e::scene::Node>& node,
    const std::function<void(const std::shared_ptr<bg2e::scene::Node>&)>& callback)
{
    callback(node);
    for (const auto& child : node->children())
    {
        visitNodes(child, callback);
    }
}
}

bool StageScene::contains(bg2e::scene::Node* node) const
{
    for (auto current = node; current; current = current->parent())
    {
        if (current == _document.root.get())
        {
            return true;
        }
    }
    return false;
}

void StageScene::prepareTools(std::shared_ptr<bg2e::scene::Node> root)
{
    // The initial environment light is a stage fixture, outside the document
    // subtree. Refresh its tools alongside document tools after loads/edits too.
    if (_fillLight && root != _fillLight)
    {
        prepareTools(_fillLight);
    }
    if (!root)
    {
        root = _document.root;
    }
    if (!root)
    {
        return;
    }
    std::set<bg2e::draw::Drawable*> drawables;
    std::set<bg2e::geo::Mesh*> meshes;
    visitNodes(root, [&](const auto& node)
    {
        using namespace bg2e;
        if (auto component = node->template getComponent<scene::DrawableComponent>())
        {
            if (auto drawable = component->drawDrawable())
            {
                if (drawables.contains(drawable.get()) || meshes.contains(drawable->mesh().get()))
                {
                    drawable = drawable->clone();
                    component->setDrawable(drawable);
                }
                drawables.insert(drawable.get());
                meshes.insert(drawable->mesh().get());
            }
        }
        if (!node->template getComponent<scene::TransformComponent>())
        {
            node->addComponent(new scene::TransformComponent());
        }
        if (!node->template getComponent<manipulation::SelectableComponent>())
        {
            auto selectable = std::make_shared<manipulation::SelectableComponent>();
            selectable->setIgnoreSerialization(true);
            node->addComponent(selectable);
        }
        node->template getComponent<manipulation::SelectableComponent>()->update(0);
        if (!node->light())
        {
            // Preserve authored transforms and geometry, but expose editor
            // gizmos only for lights in the model editor.
            if (auto gizmo = node->template getComponent<manipulation::GizmoComponent>())
            {
                if (gizmo->manipulationMode() == manipulation::ToolExecutionMode::Draw)
                {
                    gizmo->cleanupDrawResources();
                }
                node->removeComponent(gizmo->shared_from_this());
            }
            return;
        }
        if (!node->template getComponent<manipulation::GizmoComponent>())
        {
            node->addComponent(new manipulation::GizmoComponent(_engine));
        }
        node->template getComponent<manipulation::GizmoComponent>()->synchronizeResources();
    });
    _scene->updateAll();
}

void StageScene::duplicate(const std::shared_ptr<bg2e::scene::Node>& node)
{
    if (!contains(node.get()))
    {
        return;
    }
    unsigned documentLights = 0;
    unsigned copiedLights = 0;
    visitNodes(_document.root, [&](const auto& item)
    {
        documentLights += bool(item->light());
    });
    visitNodes(node, [&](const auto& item)
    {
        copiedLights += bool(item->light());
    });
    if (copiedLights && documentLights + copiedLights > 5)
    {
        throw std::invalid_argument("Duplicating this branch would exceed five document lights");
    }
    auto copy = node->clone(); // Independent CPU/GPU mesh, material and image resources.
    copy->setName(node->name() + " copy");
    if (auto transform = copy->getComponent<bg2e::scene::TransformComponent>())
    {
        transform->translate(1, 0, 0);
    }
    if (node == _document.root)
    {
        auto group = std::make_shared<bg2e::scene::Node>("Model group");
        group->addChild(copy);
        group->addChild(node->clone());
        _retired.reserve(_retired.size() + 1);
        _root->addChild(group);
        _retired.push_back(node);
        _root->removeChild(node);
        _model = group;
        _document.root = group;
        ++_document.generation;
    }
    else
    {
        node->parent()->addChild(copy);
    }
    prepareTools();
}

void StageScene::remove(const std::shared_ptr<bg2e::scene::Node>& node)
{
    if (!contains(node.get()) || node == _document.root)
    {
        throw std::invalid_argument("Select a child node to remove; the document root is retained");
    }
    _retired.reserve(_retired.size() + 1);
    _retired.push_back(node);
    node->parent()->removeChild(node);
    _scene->updateAll();
}

float StageScene::floorHeight() const
{
    return _floor ? _floor->transform()->translation().y : 0;
}

void StageScene::setFillLight(bool enabled)
{
    if (_fillLight && _fillLight->light())
    {
        _fillLight->setEnabled(enabled);
    }
    _scene->updateAll();
}

void StageScene::setFloor(bool visible, float height)
{
    if (!std::isfinite(height))
    {
        throw std::invalid_argument("Floor height must be finite");
    }
    _floor->setEnabled(visible);
    _floor->transform()->setTranslation(0, height, 0);
}

std::shared_ptr<bg2e::scene::Node> StageScene::environmentNode() const
{
    std::shared_ptr<bg2e::scene::Node> result;
    if (_document.root)
    {
        visitNodes(_document.root, [&](const auto& node)
        {
            if (!result && node->environment())
            {
                result = node;
            }
        });
    }
    return result;
}

void StageScene::adoptEnvironment(const std::shared_ptr<bg2e::scene::Node>& preset)
{
    if (!_document.root || !preset || !preset->environment())
    {
        throw std::invalid_argument("Environment preset requires an Environment root and an open document");
    }
    // Prepare clones before altering the document. Only environment/light nodes
    // belong to presets; renderer preferences are deliberately separate.
    auto environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(preset->environment()->clone());
    auto target = environmentNode();
    if (!target)
    {
        target = _document.root;
    }
    target->addComponent(environment);
    if (auto fixture = _root->environment())
    {
        _root->removeComponent(fixture->shared_from_this());
    }
    _scene->updateAll();
}

std::shared_ptr<bg2e::scene::Node> StageScene::environmentPreset() const
{
    auto result = std::make_shared<bg2e::scene::Node>("Draw editor environment");
    auto source = environmentNode();
    if (source || _root->environment())
    {
        result->addComponent(source ? source->environment()->clone() : _root->environment()->clone());
    }
    else
    {
        result->addComponent(new bg2e::scene::EnvironmentComponent());
    }
    if (_document.root)
    {
        visitNodes(_document.root, [&](const auto& node)
        {
            if (node->light())
            {
                auto light = std::make_shared<bg2e::scene::Node>(node->name());
                light->addComponent(node->light()->clone());
                light->addComponent(new bg2e::scene::TransformComponent(node->worldMatrix()));
                result->addChild(light);
            }
        });
    }
    auto floor = std::make_shared<bg2e::scene::Node>("Stage floor");
    floor->setEnabled(_floor->enabled());
    floor->addComponent(_floor->transform()->clone());
    result->addChild(floor);
    auto fill = std::make_shared<bg2e::scene::Node>("Stage fill light");
    fill->setEnabled(fillLightEnabled());
    fill->addComponent(_fillLight->transform()->clone());
    if (_fillLight->light())
    {
        fill->addComponent(_fillLight->light()->clone());
    }
    result->addChild(fill);
    return result;
}

void StageScene::validateEnvironmentPreset(const std::shared_ptr<bg2e::scene::Node>& preset) const
{
    if (!_document.root || !preset || !preset->environment())
    {
        throw std::invalid_argument("Invalid environment preset root");
    }
    for (const auto& [type, component] : preset->components())
    {
        if (!dynamic_cast<bg2e::scene::EnvironmentComponent*>(component.get()) &&
            !dynamic_cast<bg2e::scene::TransformComponent*>(component.get()))
        {
            throw std::invalid_argument("Unsupported environment preset root component");
        }
    }
    const auto determinant = glm::determinant(_document.root->worldMatrix());
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f)
    {
        throw std::invalid_argument("Cannot restore world-space lighting under a singular document transform");
    }
    unsigned lights = 0;
    for (const auto& child : preset->children())
    {
        if (!child->children().empty() || !child->transform() || (!child->light() && child->name() != "Stage floor" && child->name() != "Stage fill light"))
        {
            throw std::invalid_argument("Environment preset contains an unsupported node");
        }
        for (const auto& [type, component] : child->components())
        {
            if (!dynamic_cast<bg2e::scene::TransformComponent*>(component.get()) &&
                !dynamic_cast<bg2e::scene::LightComponent*>(component.get()))
            {
                throw std::invalid_argument("Unsupported environment preset child component");
            }
        }
        lights += child->name() != "Stage fill light" && bool(child->light());
    }
    if (lights > 5)
    {
        throw std::invalid_argument("An editor environment supports at most five lights");
    }
    const auto source = preset->environment()->environmentImage();
    if (!source.empty())
    {
        std::unique_ptr<bg2e::base::Image> pixels(bg2e::db::loadImage(source));
        if (!pixels)
        {
            throw std::invalid_argument("Could not decode the preset environment image");
        }
    }
}

void StageScene::restoreEnvironment(const std::shared_ptr<bg2e::scene::Node>& preset)
{
    validateEnvironmentPreset(preset);
    auto rig = std::make_shared<bg2e::scene::Node>("Environment rig");
    rig->addComponent(preset->environment()->clone());
    rig->addComponent(new bg2e::scene::TransformComponent(glm::inverse(_document.root->worldMatrix())));
    std::shared_ptr<bg2e::scene::Node> floor;
    for (const auto& child : preset->children())
    {
        if (child->light() && child->name() != "Stage fill light")
        {
            rig->addChild(child->clone());
        }
        else if (child->name() == "Stage floor")
        {
            floor = child;
        }
    }
    prepareTools(rig); // Resource/asset failures precede any graph mutation.
    std::vector<std::shared_ptr<bg2e::scene::Component>> old;
    visitNodes(_document.root, [&](const auto& node)
    {
        if (node->environment())
        {
            old.push_back(node->environment()->shared_from_this());
        }
        if (node->light())
        {
            old.push_back(node->light()->shared_from_this());
        }
    });
    _document.root->addChild(rig); // Allocating publication point precedes removals.
    for (const auto& component : old)
    {
        component->ownerNode()->removeComponent(component);
    }
    if (auto fixture = _root->environment())
    {
        _root->removeComponent(fixture->shared_from_this());
    }
    if (floor)
    {
        _floor->setEnabled(floor->enabled());
        _floor->transform()->setMatrix(floor->transform()->matrix());
    }
    for (const auto& child : preset->children())
    {
        if (child->name() == "Stage fill light")
        {
            if (child->light())
            {
                if (!_fillLight->light())
                {
                    _fillLight->addComponent(new bg2e::scene::LightComponent());
                }
                _fillLight->light()->light() = child->light()->light();
                _fillLight->transform()->setMatrix(child->transform()->matrix());
            }
            else if (auto light = _fillLight->light())
            {
                _fillLight->removeComponent(light->shared_from_this());
            }
            _fillLight->setEnabled(child->enabled());
            prepareTools(_fillLight);
        }
    }
    _scene->updateAll();
}

void StageScene::cleanup()
{
    bg2e::manipulation::GizmoComponent::setCurrentTransform(nullptr);
    _viewpointAnalyzer.reset();
    _camera.reset();
    _floor.reset();
    _fillLight.reset();
    std::exception_ptr error;
    try
    {
        if (_document.root)
        {
            cleanupTree(*_document.root);
        }
    }
    catch (...)
    {
        error = std::current_exception();
    }
    try
    {
        releaseRetired();
    }
    catch (...)
    {
        if (!error)
        {
            error = std::current_exception();
        }
    }
    _document.root.reset();
    _model.reset();
    _scene.reset();
    _root.reset();
    for (auto& drawable : _drawables)
    {
        try
        {
            drawable->cleanup();
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
        }
    }
    _drawables.clear();
    if (error)
    {
        std::rethrow_exception(error);
    }
}

}
