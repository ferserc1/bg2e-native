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

#include <AppDelegate.hpp>

#include <bg2e.hpp>

#include <iostream>
#include <string>

#include "bg2e/scene/FindNodeVisitor.hpp"
#include "bg2e/scene/FindCameraVisitor.hpp"
#include "bg2e/scene/FindNodeComponentVisitor.hpp"
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/geo/modifiers.hpp>

namespace {

// Drawable names double as the .bg2 asset filename written by
// DrawableComponent::serialize(). After a node is duplicated, every cloned
// drawable still carries the source's name; appending " Copy" keeps the copy's
// asset distinct so saving the scene never overwrites the source's mesh file.
// Empty names are left untouched: serialize() assigns each unnamed drawable its
// own fresh unique id, so they cannot collide.
void renameClonedDrawables(bg2e::scene::Node* node)
{
    if (!node)
    {
        return;
    }

    if (auto drawableComp = node->drawable())
    {
        auto drawable = drawableComp->drawableBase();
        if (drawable && !drawable->name().empty())
        {
            drawable->setName(drawable->name() + " Copy");
        }
    }

    for (const auto& child : node->children())
    {
        renameClonedDrawables(child.get());
    }
}

}

StageScene::StageScene(bg2e::render::Engine * engine, AppDelegate * appDelegate)
    :_engine { engine }
    ,_appDelegate { appDelegate }
    ,_document(std::make_unique<Document>(appDelegate) )
{
}

std::shared_ptr<bg2e::scene::Node> StageScene::init()
{
    _containerRoot = std::make_shared<bg2e::scene::Node>("Container");
    _editableRoot = buildDefaultScene();
    _containerRoot->addChild(_editableRoot);
    return _containerRoot;
}

std::shared_ptr<bg2e::scene::Node> StageScene::buildDefaultScene()
{
    auto scene = std::make_shared<bg2e::scene::Node>("Scene");

    // Camera node
    auto cameraNode = std::make_shared<bg2e::scene::Node>("Camera");
    cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.0f, 2.0f));
    cameraNode->addComponent(new bg2e::scene::CameraComponent());
    auto projection = new bg2e::math::OpticalProjection();
    projection->setFocalLength(35.0f);
    projection->setFrameSize(35.0f);
    projection->setFar(1000.0f);
    cameraNode->camera()->setProjection(projection);

    auto orbitCameraComp = new bg2e::scene::OrbitCameraComponent();
    orbitCameraComp->setMaxX(std::numeric_limits<float>::max());
    orbitCameraComp->setMaxY(std::numeric_limits<float>::max());
    orbitCameraComp->setMaxZ(std::numeric_limits<float>::max());
    orbitCameraComp->setMinX(-std::numeric_limits<float>::max());
    orbitCameraComp->setMinY(-std::numeric_limits<float>::max());
    orbitCameraComp->setMinZ(-std::numeric_limits<float>::max());
    orbitCameraComp->setDistance(2.0f);
    orbitCameraComp->setMaxDistance(300.0f);
    orbitCameraComp->setInitialDistance(2.0f);
    orbitCameraComp->setWheelSpeed(2.0f);
    orbitCameraComp->setPanSpeed(0.5f);
    orbitCameraComp->setRotation(glm::vec2{ 13.0f, 10.0f });
    cameraNode->addComponent(orbitCameraComp);

    scene->addChild(cameraNode);

    // Directional light node
    auto lightNode = std::make_shared<bg2e::scene::Node>("Directional Light");
    lightNode->addComponent(new bg2e::scene::TransformComponent());
    auto lightComp = new bg2e::scene::LightComponent();
    lightComp->light().setType(bg2e::base::Light::TypeDirectional);
    lightComp->light().setColor(bg2e::base::Color::White());
    lightComp->light().setIntensity(5.0f);
    lightNode->addComponent(lightComp);

    scene->addChild(lightNode);

    // Environment node
    auto environmentNode = std::make_shared<bg2e::scene::Node>("Environment");
    environmentNode->addComponent(new bg2e::scene::TransformComponent());
    environmentNode->addComponent(
        new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "mirrored_hall_4k.hdr")
    );

    scene->addChild(environmentNode);

    addGizmoComponents(scene.get());

    return scene;
}

void StageScene::setEditableRoot(std::shared_ptr<bg2e::scene::Node> newEditableRoot)
{
    _swapToken = std::make_shared<bg2e::app::SafeUpdateToken>();
    bg2e::app::MainLoop::current()->safeUpdateScene(
        [this, newEditableRoot]() {
            _appDelegate->selectionManager()->deselect();
            if (_editableRoot) _containerRoot->removeChild(_editableRoot);
            _editableRoot = newEditableRoot;
            _containerRoot->addChild(_editableRoot);
            auto scene = _containerRoot->scene();
            scene->updateAll();
            if (auto cam = cameraComponent()) scene->setMainCamera(cam);
            ensureMainCameraProjection(scene);
            addGizmoComponents(_editableRoot.get());
            if (_onSceneSwap) _onSceneSwap();
        },
        _swapToken
    );
}

void StageScene::openScene(const std::filesystem::path& path, bg2e::scene::SceneProgressCallback progressCallback)
{
    auto newScene = bg2e::db::loadScene(path, *_engine, progressCallback);

    if (!newScene || !newScene->rootNode())
    {
        bg2e::app::MessageBox msg;
        msg.showError("Error opening scene", "Could not load the specified scene file.");
        return;
    }

    // saveScene() writes _editableRoot as the single top-level node of the file,
    // but loadScene() always wraps the file's top-level nodes inside a freshly
    // created synthetic "scene root". Adopting that wrapper as the editable root
    // would nest the real root one level deeper on every save/load cycle. Since
    // saveScene() always produces exactly one top-level node, the wrapper has a
    // single child which is the real editable root: unwrap exactly one level here.
    auto wrapper = std::static_pointer_cast<bg2e::scene::Node>(
        newScene->rootNode()->shared_from_this());
    std::shared_ptr<bg2e::scene::Node> newRoot;
    if (wrapper->children().size() == 1)
    {
        newRoot = wrapper->children()[0];
        wrapper->removeChild(newRoot);
    }
    else
    {
        // Empty or multi-root files are not produced by this app, but keep the
        // wrapper as-is so no content is lost if such a file is ever opened.
        newRoot = wrapper;
    }
    setEditableRoot(newRoot);
    _document->setPath(path);
    _document->setUnsavedChanges(false);

    // Request resize event to send the first resize to the scene
    bg2e::app::MainLoop::current()->requestResizeEvent();
}

void StageScene::importModelBg2(const std::filesystem::path& path)
{
    auto drawable = bg2e::db::loadDrawableBg2(path, _engine);
    auto node = std::make_shared<bg2e::scene::Node>(path.stem().string());
    node->addComponent(new bg2e::scene::TransformComponent());
    node->addComponent(new bg2e::scene::DrawableComponent(drawable));
    node->addComponent(new bg2e::manipulation::SelectableComponent());
    node->addComponent(new bg2e::manipulation::GizmoComponent(_engine));
    auto parent = newNodeParent();
    bg2e::app::MainLoop::current()->safeUpdateScene([this, node, parent]() {
        parent->addChild(node);
        _containerRoot->scene()->updateAll();
    });
    _document->setUnsavedChanges(true);
}

std::shared_ptr<bg2e::scene::Node> StageScene::newNodeParent()
{
    // New nodes hang from the primary selected node, or from the editable root if
    // nothing is selected. selectedNode() returns the primary node when more than
    // one node is selected.
    auto selected = _appDelegate->selectionManager()->selectedNode();
    if (selected)
    {
        return std::static_pointer_cast<bg2e::scene::Node>(selected->shared_from_this());
    }
    return _editableRoot;
}

glm::vec3 StageScene::placementLocalPosition(bg2e::scene::Node* parent)
{
    auto cam = cameraComponent();
    if (!cam || !cam->ownerNode())
    {
        return glm::vec3{ 0.0f };
    }

    auto cameraNode = cam->ownerNode();
    auto cameraPos = cameraNode->worldPosition();

    // Node::forwardVector() returns the node's local +Z axis in world space. The
    // orbit camera places the eye at +Z relative to its target, so its view
    // direction is -Z; negate the forward vector to look towards the scene.
    auto viewDir = -cameraNode->forwardVector();
    auto targetWorldPos = cameraPos + viewDir * _createNodeDistance;

    // The new node may hang from a parent with its own world transform. Express
    // the target world position in the parent's local space so the node lands at
    // the right place regardless of the parent's transform.
    glm::mat4 parentWorld = parent ? parent->worldMatrix() : glm::mat4{ 1.0f };
    return glm::vec3(glm::inverse(parentWorld) * glm::vec4(targetWorldPos, 1.0f));
}

void StageScene::insertNewNode(std::shared_ptr<bg2e::scene::Node> node, std::shared_ptr<bg2e::scene::Node> parent)
{
    // Every editable node needs a SelectableComponent to be pickable in the
    // viewport and a GizmoComponent to draw its transform/type gizmo.
    if (!node->getComponent<bg2e::manipulation::SelectableComponent>())
    {
        node->addComponent(new bg2e::manipulation::SelectableComponent());
    }
    if (!node->getComponent<bg2e::manipulation::GizmoComponent>())
    {
        node->addComponent(new bg2e::manipulation::GizmoComponent(_engine));
    }

    bg2e::app::MainLoop::current()->safeUpdateScene([this, node, parent]() {
        parent->addChild(node);
        _containerRoot->scene()->updateAll();
    });
    _document->setUnsavedChanges(true);
}

void StageScene::addLightNode()
{
    if (!_editableRoot) return;

    auto parent = newNodeParent();

    auto node = std::make_shared<bg2e::scene::Node>("Light");
    auto transform = new bg2e::scene::TransformComponent();
    transform->setTranslation(placementLocalPosition(parent.get()));
    node->addComponent(transform);

    auto lightComp = new bg2e::scene::LightComponent();
    lightComp->light().setType(bg2e::base::Light::TypeOmni);
    lightComp->light().setColor(bg2e::base::Color::White());
    lightComp->light().setIntensity(5.0f);
    node->addComponent(lightComp);

    insertNewNode(node, parent);
}

void StageScene::addCubeNode()
{
    if (!_editableRoot) return;

    auto parent = newNodeParent();

    auto mesh = std::shared_ptr<bg2e::scene::Mesh>(bg2e::geo::createCube(1.0f, 1.0f, 1.0f));
    bg2e::geo::GenTangentsModifier<bg2e::scene::Mesh> genTangents(mesh.get());
    genTangents.apply();

    auto drawable = std::make_shared<bg2e::scene::Drawable>();
    drawable->setMesh(mesh);
    drawable->load(_engine);

    auto node = std::make_shared<bg2e::scene::Node>("Cube");
    auto transform = new bg2e::scene::TransformComponent();
    transform->setTranslation(placementLocalPosition(parent.get()));
    node->addComponent(transform);
    node->addComponent(new bg2e::scene::DrawableComponent(drawable));

    insertNewNode(node, parent);
}

void StageScene::addSphereNode()
{
    if (!_editableRoot) return;

    auto parent = newNodeParent();

    auto mesh = std::shared_ptr<bg2e::scene::Mesh>(bg2e::geo::createSphere(1.0f, 32, 24));
    bg2e::geo::GenTangentsModifier<bg2e::scene::Mesh> genTangents(mesh.get());
    genTangents.apply();

    auto drawable = std::make_shared<bg2e::scene::Drawable>();
    drawable->setMesh(mesh);
    drawable->load(_engine);

    auto node = std::make_shared<bg2e::scene::Node>("Sphere");
    auto transform = new bg2e::scene::TransformComponent();
    transform->setTranslation(placementLocalPosition(parent.get()));
    node->addComponent(transform);
    node->addComponent(new bg2e::scene::DrawableComponent(drawable));

    insertNewNode(node, parent);
}

void StageScene::duplicateSelectedNode()
{
    if (!_editableRoot)
    {
        return;
    }

    auto selected = _appDelegate->selectionManager()->selectedNode();
    if (!selected)
    {
        return;
    }

    auto source = std::static_pointer_cast<bg2e::scene::Node>(selected->shared_from_this());

    // The duplicate is inserted as a sibling of the source, so the source must
    // have a parent. The editable root is shown at the top of the tree and has no
    // parent inside the editable subtree: it cannot be duplicated this way.
    auto parentRaw = source->parent();
    if (!parentRaw)
    {
        return;
    }
    auto parent = std::static_pointer_cast<bg2e::scene::Node>(parentRaw->shared_from_this());

    // Deep copy the whole node: components are cloned by value (drawables build
    // their own mesh, materials and GPU resources) and the child subtree is cloned
    // recursively. Nothing is shared with the source.
    auto copy = source->clone();
    copy->setName(source->name() + " Copy");

    // Keep cloned drawable asset names distinct from the source's (see helper).
    renameClonedDrawables(copy.get());

    // The clone already carries copies of the source's Selectable/Gizmo
    // components; this also covers any node in the subtree that happened to lack
    // them so the whole duplicate stays pickable and shows its gizmos.
    addGizmoComponents(copy.get());

    insertNewNode(copy, parent);
}

void StageScene::removeSelectedNode()
{
    auto selected = _appDelegate->selectionManager()->selectedNode();
    if (!selected)
    {
        return;
    }

    auto response = bg2e::app::MessageBox::showWarning(
        "Remove selection",
        "The selected node and all its children will be removed. This action cannot be undone. Do you want to continue?",
        {
            { .code = 0, .label = "Cancel", .key = bg2e::app::MessageBox::Esc },
            { .code = 1, .label = "Remove" }
        }
    );

    if (response != 1)
    {
        return;
    }

    auto node = std::static_pointer_cast<bg2e::scene::Node>(selected->shared_from_this());
    auto parent = node->parent();
    if (!parent)
    {
        return;
    }

    // Deselect before removing: the SelectionManager keeps weak references to the
    // node and its drawable, and removing a still-selected node would leave the
    // selection highlight pointing at a destroyed object.
    _appDelegate->selectionManager()->deselect();

    bg2e::app::MainLoop::current()->safeUpdateScene([this, node, parent]() {
        parent->removeChild(node);
        _containerRoot->scene()->updateAll();
    });
    _document->setUnsavedChanges(true);
}

void StageScene::addEmptyNode()
{
    if (!_editableRoot) return;

    auto parent = newNodeParent();

    auto node = std::make_shared<bg2e::scene::Node>("Node");
    auto transform = new bg2e::scene::TransformComponent();
    transform->setTranslation(placementLocalPosition(parent.get()));
    node->addComponent(transform);

    insertNewNode(node, parent);
}

void StageScene::saveScene(const std::filesystem::path& path)
{
    // Writes _editableRoot as the single top-level node of the scene file. This
    // is the invariant openScene() relies on to unwrap loadScene()'s synthetic
    // root and avoid accumulating wrapper nodes across save/load cycles.
    bg2e::db::saveScene(_editableRoot.get(), path);
    _document->setPath(path);
    _document->setUnsavedChanges(false);
}

void StageScene::close()
{
    setEditableRoot(buildDefaultScene());
    _document->setStatus("", false);
}

void StageScene::cleanup()
{
    _editableRoot.reset();
    _containerRoot.reset();
    _document->setStatus("", false);
}

bool StageScene::checkUnsavedChanges()
{
    using namespace bg2e::app;
    if (_document->unsavedChanges())
    {
        auto response = bg2e::app::MessageBox::showWarning(
            "Unsaved changes",
            "There are unsaved changes. Do you want to save your changes before continuing?",
            {
                { .code = 0, .label = "Cancel", },
                { .code = 1, .label = "Save" },
                { .code = 2, .label = "No" }
            }
        );

        if (response == 0)
        {
            return false;
        }
        else if (response == 1)
        {
            if (_document->path().empty())
            {
                bg2e::app::FileDialog fd;
                fd.setFilters({
                    { "bg2e scene", "json,vitscnj" }
                });
                auto filePath = fd.saveFile();
                if (filePath.empty())
                {
                    return false;
                }

                _document->setPath(filePath);
            }
            saveScene(_document->path());
            return true;
        }
        else if (response == 2)
        {
            return true;
        }
    }
    return true;
}

bg2e::scene::OrbitCameraComponent * StageScene::orbitCamera()
{
    if (!_editableRoot) return nullptr;
    bg2e::scene::FindNodeComponentVisitor<bg2e::scene::OrbitCameraComponent> finder;
    auto results = finder.find(_editableRoot.get());
    if (!results.empty())
    {
        auto node = results[0].lock();
        if (node)
        {
            return node->getComponent<bg2e::scene::OrbitCameraComponent>();
        }
    }
    return nullptr;
}

void StageScene::ensureMainCameraProjection(bg2e::scene::Scene * scene)
{
    if (!scene) return;
    auto cam = scene->mainCamera();
    if (!cam || cam->projection()) return;

    auto projection = new bg2e::math::OpticalProjection();
    projection->setFrameSize(50.0f);    // film size: 50 mm
    projection->setFocalLength(55.0f);  // lens: 55 mm
    projection->setFar(1000.0f);
    cam->setProjection(projection);
}

void StageScene::addGizmoComponents(bg2e::scene::Node * root)
{
    using namespace bg2e::manipulation;

    if (!root)
    {
        return;
    }

    // Every node gets a GizmoComponent: it drives the type gizmo (camera, light,
    // environment) when applicable and, independently, the transform gizmo for
    // any node that owns a TransformComponent. Adding it to every node also lets
    // a transform gizmo appear if a TransformComponent is added later.
    if (!root->getComponent<GizmoComponent>())
    {
        root->addComponent(new GizmoComponent(_engine));
    }

    // Light/environment/camera nodes have no regular drawable, so they need a
    // SelectableComponent to be pickable through their gizmo in the viewport.
    if ((root->light() || root->environment() || root->camera()) &&
        !root->getComponent<SelectableComponent>())
    {
        root->addComponent(new SelectableComponent());
    }

    for (const auto& child : root->children())
    {
        addGizmoComponents(child.get());
    }
}

bg2e::scene::CameraComponent * StageScene::cameraComponent()
{
    if (!_editableRoot) return nullptr;
    bg2e::scene::FindCameraVisitor finder;
    finder.findCameras(_editableRoot.get());
    auto cameras = finder.cameras();
    if (!cameras.empty())
    {
        return cameras[0].lock().get();
    }
    return nullptr;
}
