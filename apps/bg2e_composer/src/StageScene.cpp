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
    auto polarCtrl = new bg2e::scene::PolarTransformControllerComponent();
    polarCtrl->setAzimuth(225.0f);
    polarCtrl->setElevation(56.27f);
    polarCtrl->setDistance(3.33f);
    polarCtrl->setEulerX(58.27f);
    polarCtrl->setEulerY(40.0f);
    lightNode->addComponent(polarCtrl);

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

void StageScene::openScene(const std::filesystem::path& path)
{
    auto newScene = bg2e::db::loadScene(path, *_engine);
    if (!newScene || !newScene->rootNode())
    {
        bg2e::app::MessageBox msg;
        msg.showError("Error opening scene", "Could not load the specified scene file.");
        return;
    }
    auto newRoot = std::static_pointer_cast<bg2e::scene::Node>(
        newScene->rootNode()->shared_from_this());
    setEditableRoot(newRoot);
    _document->setPath(path);
    _document->setUnsavedChanges(false);
}

void StageScene::importModelBg2(const std::filesystem::path& path)
{
    auto drawable = bg2e::db::loadDrawableBg2(path, _engine);
    auto node = std::make_shared<bg2e::scene::Node>(path.stem().string());
    node->addComponent(new bg2e::scene::TransformComponent());
    node->addComponent(new bg2e::scene::DrawableComponent(drawable));
    node->addComponent(new bg2e::manipulation::SelectableComponent());
    bg2e::app::MainLoop::current()->safeUpdateScene([this, node]() {
        _editableRoot->addChild(node);
        _containerRoot->scene()->updateAll();
    });
    _document->setUnsavedChanges(true);
}

void StageScene::saveScene(const std::filesystem::path& path)
{
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
    using namespace bg2e::scene;

    bg2e::scene::FindNodeComponentVisitor<CameraComponent> cameraFinder;
    for (auto& wp : cameraFinder.find(root))
    {
        if (auto node = wp.lock())
        {
            if (!node->getComponent<GizmoComponent>())
                node->addComponent(new GizmoComponent(_engine));
        }
    }

    bg2e::scene::FindNodeComponentVisitor<LightComponent> lightFinder;
    for (auto& wp : lightFinder.find(root))
    {
        if (auto node = wp.lock())
        {
            if (!node->getComponent<GizmoComponent>())
                node->addComponent(new GizmoComponent(_engine));
        }
    }

    bg2e::scene::FindNodeComponentVisitor<EnvironmentComponent> envFinder;
    for (auto& wp : envFinder.find(root))
    {
        if (auto node = wp.lock())
        {
            if (!node->getComponent<GizmoComponent>())
                node->addComponent(new GizmoComponent(_engine));
        }
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
