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
#include <bg2e/db/scene_path.hpp>
#include <bg2e/db/scene_save_dialog.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/db/scene_bg2.hpp>
#include <bg2e/db/scene_gltf.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/geo/plane.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/math/projections.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/BoundingBox.hpp>
#include <bg2e/scene/FindCameraVisitor.hpp>
#include <bg2e/scene/UpdateVisitor.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/utils/SceneCameraUtils.hpp>

#include <exception>
#include <cmath>
#include <set>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cctype>

namespace bg2e_composer {
namespace {
unsigned enabledLightCount(bg2e::scene::Node* root)
{
    if (!root || !root->enabled())
    {
        return 0;
    }
    unsigned count = root->light() ? 1u : 0u;
    for (const auto& child : root->children())
    {
        count += enabledLightCount(child.get());
    }
    return count;
}


}

StageScene::StageScene(bg2e::draw::Engine* engine) : _engine(engine)
{
    if (!_engine)
    {
        throw std::invalid_argument("The composer stage requires a draw Engine");
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::create(std::shared_ptr<command::Dispatcher> commands)
{
    using namespace bg2e;
    if (_root)
    {
        throw std::logic_error("The composer stage is already initialized");
    }
    if (!commands || commands->history().documentState().generation != 1)
    {
        throw std::invalid_argument("The initial stage requires a generation-one dispatcher");
    }
    _root = std::make_shared<scene::Node>("Composer stage");

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
    auto startup = _root->children().back();
    _root->removeChild(startup);
    _model = std::make_shared<scene::Node>("Scene");
    _model->addComponent(new scene::TransformComponent());
    _model->addChild(startup);
    _root->addChild(_model);
    _document.root = _model;
    _document.generation = 1;
    _document.commands = std::move(commands);
    _document.synchronizeDirty();

    auto floor = std::make_shared<draw::Drawable>();
    floor->setMesh(geo::createPlane(20, 20));
    floor->setRayTracingEnabled(false); // Editor fixture, excluded from future bake snapshots.
    floor->material().setAlbedo(base::Color(0.4f, 0.4f, 0.4f));
    floor->material().setMetalness(0);
    floor->material().setRoughness(0.8f);
    floor->material().setAlphaCutoff(0);
    addDrawable("Floor", floor, {0, -1.05f, 0});
    _floor = _root->children().back();
    _floor->setEnabled(false); // Composer displays authored scene content, not Model Edit fixtures.

    _camera = std::make_shared<scene::Node>("Editor camera");
    _camera->addComponent(new scene::TransformComponent());
    _camera->addComponent(new scene::OrbitCameraComponent());
    _camera->addComponent(new scene::CameraComponent());
    _camera->camera()->setProjection(new bg2e::math::PerspectiveProjection());
    auto* orbit = _camera->getComponent<scene::OrbitCameraComponent>();
    orbit->setWheelSpeed(0.4f);
    orbit->setRotateButtons(true, false, false);
    orbit->setPanButtons(false, false, true);
    orbit->setZoomButtons(false, true, false);
    _model->addChild(_camera);
    resetCamera();

    auto light = std::make_shared<scene::Node>("Stage light");
    light->addComponent(scene::TransformComponent::makeTranslated(2, 2, 2));
    light->addComponent(new scene::LightComponent());
    light->light()->light().setType(base::Light::TypeOmni);
    light->light()->light().setIntensity(45);
    light->light()->light().setCastShadows(true);
    _root->addChild(light);
    _fillLight = light;
    _fillLight->setEnabled(false);
    _scene = std::make_shared<scene::Scene>();
    _scene->setSceneRoot(_root);
    _scene->setMainEnvironment(nullptr);
    _scene->setMainCamera(_camera->camera());
    prepareTools();
    refreshCamera();
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

std::shared_ptr<bg2e::scene::Node> StageScene::createPrimitive(unsigned kind)
{
    std::shared_ptr<bg2e::geo::MeshPNUUT> mesh;
    const char* name = nullptr;
    switch (kind)
    {
    case 0:
        name = "Cube";
        mesh.reset(bg2e::geo::createCube(1.0f, 1.0f, 1.0f));
        break;
    case 1:
        name = "Sphere";
        mesh.reset(bg2e::geo::createSphere(0.3f, 32, 16));
        break;
    case 2:
        name = "Plane";
        mesh.reset(bg2e::geo::createPlane(10.0f, 10.0f));
        break;
    default:
        throw std::invalid_argument("Unknown scene primitive");
    }

    auto drawable = std::make_shared<bg2e::draw::Drawable>();
    drawable->setMesh(std::move(mesh));
    _drawables.push_back(drawable); // Retain partial GPU initialization for stage cleanup.
    drawable->load(_engine);

    auto node = std::make_shared<bg2e::scene::Node>(name);
    node->addComponent(new bg2e::scene::TransformComponent());
    node->addComponent(new bg2e::scene::DrawableComponent(std::move(drawable)));
    return node;
}

void StageScene::resetCamera()
{
    if (_camera)
    {
        auto* orbit = orbitCamera();
        if (!orbit)
        {
            return;
        }
        orbit->setCenter({0, 0, 0});
        orbit->setDistance(8);
        orbit->setRotation({15.0f, 30.0f});
    }
}

bg2e::scene::OrbitCameraComponent* StageScene::orbitCamera() const
{
    for (auto node = _camera.get(); node && contains(node); node = node->parent())
    {
        if (auto* orbit = node->getComponent<bg2e::scene::OrbitCameraComponent>())
        {
            return orbit;
        }
    }
    return nullptr;
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
    bg2e::utils::CameraCandidate candidate;
    candidate.target = bounds.center();
    candidate.distance = distance;
    const auto direction = glm::normalize(_camera->worldPosition() - candidate.target);
    candidate.position = candidate.target + direction * distance;
    applyViewpoint(candidate);
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
    auto orbit = orbitCamera();
    if (!orbit || !orbit->ownerNode())
    {
        throw std::invalid_argument("Viewpoint actions require an authored orbit controller");
    }
    auto local = candidate;
    auto parent = orbit->ownerNode()->parent();
    const auto matrix = parent ? parent->worldMatrix() : glm::mat4(1);
    const glm::vec3 x(matrix[0]), y(matrix[1]), z(matrix[2]);
    const float scale = glm::length(x);
    // An orbit cannot represent shear, reflection or nonuniform parent scale.
    // Reject these without touching the authored pose.
    if (!std::isfinite(scale) || scale < 1e-6f || glm::determinant(glm::mat3(matrix)) <= 0 ||
        std::abs(glm::length(y) - scale) > scale * 1e-4f ||
        std::abs(glm::length(z) - scale) > scale * 1e-4f ||
        std::abs(glm::dot(x, y)) > scale * scale * 1e-4f ||
        std::abs(glm::dot(x, z)) > scale * scale * 1e-4f ||
        std::abs(glm::dot(y, z)) > scale * scale * 1e-4f)
    {
        throw std::invalid_argument("Viewpoint actions require a rigid or uniformly scaled camera parent");
    }
    const auto inverse = glm::inverse(matrix);
    local.target = glm::vec3(inverse * glm::vec4(candidate.target, 1));
    local.position = glm::vec3(inverse * glm::vec4(candidate.position, 1));
    const auto delta = local.position - local.target;
    local.distance = glm::length(delta);
    if (!std::isfinite(local.distance) || local.distance < 1e-6f)
    {
        throw std::invalid_argument("Invalid viewpoint distance");
    }
    local.pitch = glm::degrees(std::asin(std::clamp(delta.y / local.distance, -1.0f, 1.0f)));
    local.yaw = glm::degrees(std::atan2(delta.x, delta.z));
    bg2e::utils::applyViewpointToOrbitCamera(local, orbit, camera());
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
    if (auto component = node.drawable())
    {
        if (auto drawable = component->drawable())
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

void StageScene::cleanupRetainedNode(const std::shared_ptr<bg2e::scene::Node>& node)
{
    if (node && !node->parent() && !node->scene())
    {
        cleanupTree(*node);
    }
}


void StageScene::discardCandidate(const std::shared_ptr<bg2e::scene::Node>& candidate) noexcept
{
    if (!candidate || candidate->parent() || candidate->scene())
    {
        return;
    }
    try
    {
        cleanupTree(*candidate);
    }
    catch (...)
    {
        // Preserve the original load/adoption exception; detached RAII resources
        // are still released when the last candidate reference is destroyed.
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::newDocumentRoot() const
{
    using namespace bg2e;
    auto root = std::make_shared<scene::Node>("Scene");
    root->addComponent(new scene::TransformComponent());
    auto camera = std::make_shared<scene::Node>("Camera");
    camera->addComponent(new scene::TransformComponent());
    camera->addComponent(new scene::CameraComponent());
    camera->camera()->setProjection(new bg2e::math::PerspectiveProjection());
    auto orbit = std::make_shared<scene::OrbitCameraComponent>();
    orbit->setDistance(8);
    orbit->setRotation({15, 30});
    orbit->setWheelSpeed(0.4f);
    camera->addComponent(orbit);
    root->addChild(camera);
    return root;
}

void StageScene::prepareCameras(const std::shared_ptr<bg2e::scene::Node>& root, bool allowFallback) const
{
    using namespace bg2e;
    if (auto* camera = root->camera())
    {
        auto* projection = camera->projection();
        if (!projection)
        {
            if (!allowFallback)
            {
                throw std::invalid_argument("An authored camera must retain a valid projection");
            }
            auto fallback = std::make_shared<math::OpticalProjection>();
            fallback->setFrameSize(50);
            fallback->setFocalLength(55);
            fallback->setFar(1000);
            camera->setProjection(fallback);
        }
        const auto* prepared = camera->projection();
        if (!std::isfinite(prepared->near()) || !std::isfinite(prepared->far()) ||
            prepared->near() <= 0 || prepared->far() <= prepared->near())
        {
            throw std::invalid_argument("Camera requires finite clipping planes with 0 < near < far");
        }
        float fov = utils::cameraVerticalFieldOfView(camera);
        if (auto perspective = camera->projection<math::PerspectiveProjection>())
        {
            fov = perspective->fov();
        }
        else if (auto optical = camera->projection<math::OpticalProjection>())
        {
            if (!std::isfinite(optical->frameSize()) || !std::isfinite(optical->focalLength()) ||
                optical->frameSize() <= 0 || optical->focalLength() <= 0)
            {
                throw std::invalid_argument("Optical camera requires finite positive film size and focal length");
            }
            fov = glm::degrees(2.0f * std::atan(optical->frameSize() / (2.0f * optical->focalLength())));
        }
        if (!std::isfinite(fov) || fov <= 0 || fov >= 179)
        {
            throw std::invalid_argument("Camera requires a finite vertical field of view in (0, 179)");
        }
    }
    for (const auto& child : root->children())
    {
        prepareCameras(child, allowFallback);
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::prepareScene(const std::filesystem::path& path,
    bool requireCamera, bg2e::scene::SceneProgressCallback progress,
    std::shared_ptr<bg2e::json::JsonNode> schema, bool realizeResources)
{
    // CPU schema parsing may already have completed on the import worker.
    auto loaded = schema ? bg2e::scene::Scene::deserialize(schema, path.parent_path()) :
        bg2e::db::loadScene(path, progress);
    if (!loaded || !loaded->rootNode())
    {
        throw std::invalid_argument("The file does not contain a scene");
    }
    auto wrapper = loaded->releaseSceneRoot();
    std::shared_ptr<bg2e::scene::Node> candidate = wrapper;
    if (wrapper->children().size() == 1)
    {
        candidate = wrapper->children().front();
        wrapper->removeChild(candidate); // Unwrap the synthetic level exactly once.
    }
    if (requireCamera)
    {
        bg2e::scene::FindCameraVisitor cameras;
        cameras.setIgnoreDisabled(true);
        cameras.findCameras(candidate.get());
        if (cameras.cameras().empty())
        {
            throw std::invalid_argument("A Composer scene requires an enabled camera; use Import for a prefab");
        }
    }
    prepareCameras(candidate);
    if (requireCamera && enabledLightCount(candidate.get()) > 8)
    {
        throw std::invalid_argument("Draw scenes support at most eight enabled lights");
    }
    if (!realizeResources)
    {
        return candidate;
    }
    bg2e::scene::SceneLoadProgress resources;
    resources.callback = std::move(progress);
    try
    {
        candidate->loadResources(*_engine, &resources);
        return candidate;
    }
    catch (...)
    {
        discardCandidate(candidate);
        throw;
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::load(const std::filesystem::path& input,
    std::shared_ptr<bg2e::json::JsonNode> schema, bool realizeResources)
{
    auto path = std::filesystem::absolute(input).lexically_normal();
    if (std::filesystem::is_directory(path))
    {
        path = bg2e::db::resolveSceneLoadPath(path);
    }
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    if (extension == ".json" || extension == ".bg2scene" || extension == ".bg2e" || extension == ".vitscnj")
    {
        return prepareScene(path, true, nullptr, std::move(schema), realizeResources);
    }
    auto root = newDocumentRoot();
    root->addChild(prepareImport(path));
    return root;
}

std::shared_ptr<bg2e::scene::Node> StageScene::prepareImport(const std::filesystem::path& input,
    float unitsScale, bool sourceIsZUp, bg2e::scene::SceneProgressCallback progress,
    const bg2e::db::GltfDocument* gltf, std::shared_ptr<bg2e::json::JsonNode> schema,
    const bg2e::db::Bg2Mesh* bg2, const bg2e::db::ObjMesh* obj)
{
    using namespace bg2e;
    if (!std::isfinite(unitsScale) || unitsScale <= 0)
    {
        throw std::invalid_argument("Import units scale must be finite and positive");
    }
    auto path = std::filesystem::absolute(input).lexically_normal();
    if (std::filesystem::is_directory(path))
    {
        path = bg2e::db::resolveSceneLoadPath(path);
    }
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    std::shared_ptr<scene::Node> node;
    if (extension == ".bg2" || extension == ".vwglb")
    {
        node = bg2 ? db::loadSceneBg2(path, *bg2, *_engine, progress) : db::loadSceneBg2(path, *_engine, progress);
    }
    else if (extension == ".gltf" || extension == ".glb")
    {
        node = gltf ? db::loadGltf(*gltf, *_engine, progress) : db::loadGltf(path, *_engine, progress);
    }
    else if (extension == ".obj")
    {
        node = std::make_shared<scene::Node>(path.stem().string());
        auto component = std::make_shared<scene::DrawableComponent>(obj ?
            db::loadDrawableObj(*obj, *_engine) : db::loadDrawableObj(path, *_engine));
        component->setResourcePath(path);
        node->addComponent(component);
    }
    else if (extension == ".json" || extension == ".bg2scene" || extension == ".bg2e" || extension == ".vitscnj")
    {
        node = prepareScene(path, false, progress, std::move(schema));
    }
    else
    {
        throw std::invalid_argument("Supported imports: BG2, VWGLB, OBJ/MTL, glTF/GLB and scene prefab");
    }
    if (!node)
    {
        throw std::runtime_error("Import returned no scene content");
    }
    auto wrapper = std::make_shared<scene::Node>(path.stem().string());
    try
    {
        prepareCameras(node);
        auto matrix = glm::mat4(1);
        if (sourceIsZUp)
        {
            matrix = glm::rotate(matrix, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        }
        matrix = glm::scale(matrix, glm::vec3(unitsScale));
        wrapper->addComponent(new scene::TransformComponent(matrix));
        wrapper->addChild(node);
        prepareTools(wrapper);
        return wrapper;
    }
    catch (...)
    {
        if (!node->parent())
        {
            discardCandidate(node);
        }
        discardCandidate(wrapper);
        throw;
    }
}

void StageScene::validateEditingGraph(bool prepareCameraState)
{
    if (enabledLightCount(_document.root.get()) > 8)
    {
        throw std::invalid_argument("Draw scenes support at most eight enabled lights");
    }
    bg2e::scene::FindCameraVisitor cameras;
    cameras.setIgnoreDisabled(true);
    cameras.findCameras(_document.root.get());
    if (cameras.cameras().empty())
    {
        throw std::invalid_argument("Keep at least one enabled document camera");
    }
    prepareCameras(_document.root, prepareCameraState);
}

void StageScene::refreshCamera(bool preserveControllers)
{
    refreshEnvironment();
    if (_fillLight && enabledLightCount(_document.root.get()) >= 8)
    {
        _fillLight->setEnabled(false);
    }
    bg2e::scene::FindCameraVisitor find;
    find.setIgnoreDisabled(true);
    find.findCameras(_document.root.get());
    for (const auto& weak : find.cameras())
    {
        if (auto camera = weak.lock())
        {
            auto next = camera->ownerNode()->shared_from_this();
            if (_camera != next && !preserveControllers)
            {
                // Controller clones retain authored settings and clear transient
                // pressed-button state before another camera starts receiving input.
                auto resetControllers = [&](bg2e::scene::Node* node)
                {
                    for (; node && contains(node); node = node->parent())
                    {
                        if (auto controller = node->getComponent<bg2e::scene::OrbitCameraComponent>())
                        {
                            auto fresh = controller->clone();
                            fresh->setPriority(controller->priority());
                            fresh->setIgnoreSerialization(controller->ignoreSerialization());
                            node->removeComponent(controller->shared_from_this());
                            node->addComponent(fresh);
                        }
                    }
                };
                resetControllers(_camera.get());
                resetControllers(next.get());
            }
            _camera = std::move(next);
            _scene->updateLights();
            _scene->updateEnvironment();
            _scene->setMainCamera(camera.get());
            return;
        }
    }
    throw std::logic_error("The editable scene has no enabled camera");
}

StageScene::EditingState StageScene::editingState() const
{
    EditingState state;
    state.camera = _camera;
    state.explicitEnvironment = _scene->hasExplicitMainEnvironment();
    if (state.explicitEnvironment)
    {
        if (auto environment = _scene->mainEnvironment())
        {
            state.environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(environment->shared_from_this());
        }
    }
    state.fillEnabled = _fillLight && _fillLight->enabled();
    state.knownEnvironments = _knownEnvironmentNodes;
    return state;
}


void StageScene::restoreEditingState(const EditingState& state)
{
    auto camera = state.camera.lock();
    auto environment = state.environment.lock();
    if (!camera || !contains(camera.get()) || !camera->camera() ||
        (environment && !contains(environment->ownerNode())))
    {
        throw std::logic_error("Cannot restore stale scene references");
    }
    // Copy before changing any active reference; allocation can fail safely.
    auto known = state.knownEnvironments;
    _scene->setMainCamera(camera->camera());
    if (state.explicitEnvironment)
    {
        _scene->setMainEnvironment(environment.get());
    }
    else
    {
        _scene->resetMainEnvironmentSelection();
    }
    _camera = std::move(camera);
    _knownEnvironmentNodes = std::move(known);
    if (_fillLight)
    {
        _fillLight->setEnabled(state.fillEnabled);
    }
    _scene->updateLights();
    if (state.explicitEnvironment)
    {
        _scene->updateEnvironment();
    }
}


size_t StageScene::editingStateBytes() const
{
    size_t bytes = sizeof(EditingState);
    // Upper bound for environment-map nodes/keys in both before and after.
    std::function<void(const std::shared_ptr<bg2e::scene::Node>&)> count;
    count = [&](const auto& node)
    {
        bytes += sizeof(std::pair<const std::string, std::weak_ptr<bg2e::scene::EnvironmentComponent>>);
        bytes += node->identifier().capacity() + 4 * sizeof(void*);
        for (const auto& child : node->children())
        {
            count(child);
        }
    };
    count(_document.root);
    return bytes;
}


void StageScene::adopt(std::shared_ptr<bg2e::scene::Node> candidate, const std::filesystem::path& source,
    std::shared_ptr<command::Dispatcher> commands, std::function<void(bool restoring)> rebind)
{
    if (!candidate || candidate->parent() || candidate->scene())
    {
        throw std::invalid_argument("Cannot adopt an empty or attached scene");
    }
    bg2e::scene::FindCameraVisitor cameras;
    cameras.setIgnoreDisabled(true);
    cameras.findCameras(candidate.get());
    if (cameras.cameras().empty())
    {
        throw std::invalid_argument("A Composer document requires an enabled camera");
    }
    prepareCameras(candidate);
    const unsigned totalLights = enabledLightCount(candidate.get());
    if (totalLights > 8)
    {
        throw std::invalid_argument("Draw scenes support at most eight enabled lights");
    }
    Document next;
    next.root = candidate;
    next.sourcePath = source.empty() ? source : std::filesystem::absolute(source).lexically_normal();
    if (!source.empty() && std::filesystem::is_directory(next.sourcePath))
    {
        next.sourcePath = bg2e::db::resolveSceneLoadPath(next.sourcePath);
    }
    const auto extension = next.sourcePath.extension().string();
    next.dirty = !source.empty() && extension != ".json" && extension != ".bg2scene" && extension != ".bg2e" && extension != ".vitscnj";
    if (!source.empty() && !next.dirty)
    {
        next.savePath = next.sourcePath;
    }
    next.generation = _document.generation + 1;
    if (!commands || commands->history().documentState().generation != next.generation)
    {
        throw std::invalid_argument("Replacement requires a prepared command dispatcher");
    }
    next.commands = std::move(commands);
    if (next.dirty)
    {
        next.commands->history().noteNonhistoricalChange(bg2e::app::CommandCompatibility::Preserve);
    }
    next.synchronizeDirty();
    // Keep the original document/history and stage order until publication is
    // known to be usable. Failures before this point never touch either.
    auto previousDocument = _document;
    auto previousModel = _model;
    const auto previousChildren = _root->children();
    const auto previousState = editingState();
    _retired.reserve(_retired.size() + 2);
    _root->addChild(candidate);
    try
    {
        if (previousModel)
        {
            _root->removeChild(previousModel);
        }
        _model = candidate;
        _document = std::move(next);
        refreshEnvironment(true);
        if (totalLights == 8)
        {
            _fillLight->setEnabled(false);
        }
        refreshCamera(true);
        if (rebind)
        {
            rebind(false);
        }
    }
    catch (...)
    {
        try
        {
            _root->removeChild(candidate);
            for (const auto& child : previousChildren)
            {
                _root->removeChild(child);
            }
            for (const auto& child : previousChildren)
            {
                _root->addChild(child);
            }
            _model = previousModel;
            _document = std::move(previousDocument);
            restoreEditingState(previousState);
            if (rebind)
            {
                rebind(true);
            }
        }
        catch (...)
        {
            // Neither publication can be assumed usable. Preserve both branches
            // until renderer-first teardown, and explicitly block history replay.
            if (previousModel)
            {
                _retired.push_back(previousModel);
            }
            _retired.push_back(candidate);
            if (_document.commands)
            {
                _document.commands->history().reportExternalFault();
                _document.synchronizeDirty();
            }
            throw;
        }
        throw;
    }
    if (previousModel)
    {
        _retired.push_back(previousModel);
    }
}

bool StageScene::save(const std::filesystem::path& path, bool destinationConfirmed)
{
    if (!_document.root)
    {
        throw std::logic_error("Open a document before saving");
    }
    if (_document.commands && (_document.commands->pending() ||
        _document.commands->history().transactionActive() || _document.commands->history().faulted()))
    {
        throw std::logic_error("Flush and settle document commands before saving");
    }
    const auto target = bg2e::db::inspectSceneSaveTarget(path);
    const bool current = !target.pathChanged && target.path == _document.savePath;
    auto committedPath = bg2e::db::confirmSceneSavePath(path, bg2e::db::SceneSavePathSource::Direct,
        bg2e::db::SceneArtifactType::Scene,
        destinationConfirmed || current ? bg2e::db::SceneOverwritePolicy::Allow : bg2e::db::SceneOverwritePolicy::Confirm);
    if (committedPath.empty())
    {
        return false;
    }
    refreshCamera();
    bg2e::scene::UpdateVisitor update;
    update.update(_document.root.get(), 0);
    if (!bg2e::db::saveScene(_document.root.get(), *_engine, committedPath))
    {
        return false;
    }
    // Commit metadata only after complete resource/main publication succeeds.
    if (_document.commands)
    {
        const auto saved = _document.commands->history().markSaved();
        if (!saved.accepted())
        {
            throw std::logic_error("Document history was not settled before saving");
        }
    }
    _document.savePath = std::move(committedPath);
    _document.synchronizeDirty();
    return true;
}

bool StageScene::exportModels(const std::vector<std::shared_ptr<bg2e::scene::Node>>& nodes,
    const std::filesystem::path& destination, bool batch)
{
    using namespace bg2e;
    if (destination.empty() || nodes.empty() || (!batch && nodes.size() != 1))
    {
        throw std::invalid_argument("Model export requires an output and explicit selected nodes");
    }
    std::vector<std::pair<std::shared_ptr<scene::Node>, std::filesystem::path>> exports;
    std::set<std::string> names;
    // Validate every target/name before publishing the first file. Batch mode
    // depends on selection count; non-drawable selections are skipped only there.
    for (const auto& node : nodes)
    {
        if (!node || !contains(node.get()))
        {
            throw std::invalid_argument("Model export target is no longer in the document");
        }
        auto component = node->drawable();
        if (!component || !component->drawable())
        {
            if (!batch)
            {
                throw std::invalid_argument("The selected node does not own a draw model");
            }
            continue;
        }
        auto output = destination;
        if (batch)
        {
            const auto& name = node->name();
            if (name.empty() || name == "." || name == ".." || name.back() == '.' || name.back() == ' ' ||
                name.find_first_of("/\\:<>\"|?*") != std::string::npos ||
                std::any_of(name.begin(), name.end(), [](unsigned char c)
                {
                    return c < 32;
                }))
            {
                throw std::invalid_argument("Model name is not a portable export directory: " + name);
            }
            auto key = name;
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::toupper(c));
            });
            const auto stem = key.substr(0, key.find('.'));
            if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
                (stem.size() == 4 && (stem.starts_with("COM") || stem.starts_with("LPT")) && stem[3] >= '1' && stem[3] <= '9') ||
                !names.insert(key).second)
            {
                throw std::invalid_argument("Reserved or duplicate model export name: " + name);
            }
            output /= name;
            if (std::filesystem::exists(output))
            {
                throw std::invalid_argument("Batch export directory already exists: " + output.string());
            }
            output /= name + ".bg2";
        }
        else if (output.extension() != ".bg2")
        {
            throw std::invalid_argument("Export a model with a .bg2 extension");
        }
        // Export this node's model only: no descendants, scene components or
        // placement transform. The live drawable is borrowed for snapshot IO.
        auto model = std::make_shared<scene::Node>(node->name());
        model->addComponent(new scene::DrawableComponent(component->drawable()));
        if (auto joint = node->getComponent<scene::InputChainJointComponent>())
        {
            model->addComponent(joint->clone());
        }
        if (auto joint = node->getComponent<scene::OutputChainJointComponent>())
        {
            model->addComponent(joint->clone());
        }
        exports.emplace_back(std::move(model), std::move(output));
    }
    if (exports.empty())
    {
        throw std::invalid_argument("No selected node owns a draw model");
    }
    for (const auto& [model, output] : exports)
    {
        // Each complete writer is transactional. A later batch failure leaves
        // earlier exported models intact and reports the failed destination.
        if (!db::storeDrawableBg2(output, *_engine, model.get()))
        {
            return false;
        }
    }
    return true;
}

std::shared_ptr<command::SceneRetention> StageScene::retainForCommand(const std::shared_ptr<bg2e::scene::Node>& node)
{
    for (const auto& lease : _commandRetained)
    {
        if (lease->node == node)
        {
            return lease;
        }
    }
    auto lease = std::make_shared<command::SceneRetention>(node);
    _commandRetained.push_back(lease);
    return lease;
}


std::shared_ptr<command::SceneRetention> StageScene::retainForCommand(const std::shared_ptr<bg2e::scene::Component>& component)
{
    for (const auto& lease : _commandRetained)
    {
        if (lease->component == component)
        {
            return lease;
        }
    }
    auto lease = std::make_shared<command::SceneRetention>(component);
    _commandRetained.push_back(lease);
    return lease;
}


std::shared_ptr<command::SceneRetention> StageScene::retainPreparedEnvironment(const std::shared_ptr<bg2e::scene::Node>& node)
{
    auto lease = std::make_shared<command::SceneRetention>(node, true);
    _commandRetained.push_back(lease);
    return lease;
}


void StageScene::releaseRetired()
{
    std::exception_ptr error;
    const auto heldByHistory = [&](bg2e::scene::Node* root)
    {
        for (const auto& lease : _commandRetained)
        {
            if (lease.use_count() <= 1)
            {
                continue;
            }
            auto* node = lease->node ? lease->node.get() : lease->component->ownerNode();
            for (; node; node = node->parent())
            {
                if (node == root)
                {
                    return true;
                }
            }
        }
        return false;
    };
    for (auto it = _commandRetained.begin(); it != _commandRetained.end();)
    {
        if (it->use_count() != 1)
        {
            ++it;
            continue;
        }
        if ((*it)->node && heldByHistory((*it)->node.get()))
        {
            ++it;
            continue;
        }
        if ((*it)->detached())
        {
            _engine->device()->waitIdle();
            (*it)->release(*this);
        }
        it = _commandRetained.erase(it);
    }
    for (auto it = _retired.begin(); it != _retired.end();)
    {
        if (heldByHistory(it->get()))
        {
            ++it;
            continue;
        }
        try
        {
            _engine->device()->waitIdle();
            cleanupTree(**it);
            it = _retired.erase(it);
        }
        catch (...)
        {
            if (!error)
            {
                error = std::current_exception();
            }
            ++it;
        }
    }
    // Primitive construction retains partial resources until a stopped cleanup.
    // Once cleanup has cleared the engine binding, the cache must not keep CPU
    // geometry/settings alive after the last command lease is retired.
    std::erase_if(_drawables, [](const auto& drawable)
    {
        return !drawable->isLoaded() && !drawable->engine();
    });
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
    const bool refresh = !root;
    if (refresh && _fillLight)
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
        if (auto component = node->drawable())
        {
            if (auto drawable = component->drawable())
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
        if (!node->template getComponent<manipulation::SelectableComponent>())
        {
            auto selectable = std::make_shared<manipulation::SelectableComponent>();
            selectable->setIgnoreSerialization(true);
            node->addComponent(selectable);
        }
        node->template getComponent<manipulation::SelectableComponent>()->update(0);
        if (!node->template getComponent<manipulation::GizmoComponent>())
        {
            node->addComponent(new manipulation::GizmoComponent(_engine));
        }
        node->template getComponent<manipulation::GizmoComponent>()->synchronizeResources();
    });
    if (refresh)
    {
        refreshCamera(true);
    }
}

void StageScene::prepareCommandCandidate(const std::shared_ptr<bg2e::scene::Node>& node)
{
    command::requireSupportedTree(node);
    prepareCameras(node);
    prepareTools(node);
}


float StageScene::floorHeight() const
{
    return _floor ? _floor->transform()->translation().y : 0;
}

std::shared_ptr<bg2e::scene::Node> StageScene::environmentNode() const
{
    auto* environment = _scene ? _scene->mainEnvironment() : nullptr;
    return environment ? environment->ownerNode()->shared_from_this() : nullptr;
}

void StageScene::refreshEnvironment(bool reset)
{
    if (!_scene)
    {
        return;
    }
    if (reset)
    {
        _scene->resetMainEnvironmentSelection();
    }
    std::vector<std::shared_ptr<bg2e::scene::Node>> eligible;
    std::function<void(const std::shared_ptr<bg2e::scene::Node>&)> collect;
    collect = [&](const auto& node)
    {
        if (!node || !node->enabled())
        {
            return;
        }
        if (node->environment())
        {
            eligible.push_back(node);
        }
        for (const auto& child : node->children())
        {
            collect(child);
        }
    };
    collect(_document.root);
    const bool hadSelection = _scene->hasExplicitMainEnvironment();
    auto* active = hadSelection ? _scene->mainEnvironment() : nullptr;
    std::map<std::string, std::weak_ptr<bg2e::scene::EnvironmentComponent>> known;
    for (const auto& node : eligible)
    {
        auto environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(node->environment()->shared_from_this());
        known.emplace(node->identifier(), environment);
        const auto previous = _knownEnvironmentNodes.find(node->identifier());
        const bool added = previous == _knownEnvironmentNodes.end() || previous->second.lock() != environment;
        if (!active && (reset || added))
        {
            active = node->environment();
        }
    }
    if (active || reset || hadSelection)
    {
        _scene->setMainEnvironment(active);
    }
    _knownEnvironmentNodes = std::move(known);
}

void StageScene::environmentComponentReplaced(bg2e::scene::Node& node)
{
    const auto known = _knownEnvironmentNodes.find(node.identifier());
    if (known != _knownEnvironmentNodes.end())
    {
        auto* environment = node.environment();
        known->second = environment ?
            std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(environment->shared_from_this()) : nullptr;
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::environmentPreset() const
{
    auto result = std::make_shared<bg2e::scene::Node>("Draw editor environment");
    auto source = environmentNode();
    if (source)
    {
        result->addComponent(source->environment()->clone());
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
    _knownEnvironmentNodes.clear();
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
