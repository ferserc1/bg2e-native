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

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "bg2e/scene/FindNodeVisitor.hpp"

StageScene::StageScene(bg2e::render::Engine * engine, AppDelegate * appDelegate)
    :_engine { engine }
    ,_appDelegate { appDelegate }
    ,_document(std::make_unique<Document>(appDelegate) )
{
}

std::shared_ptr<bg2e::scene::Node> StageScene::init()
{
    auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
    
    auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
    cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.0f, 2.0f ));
    
    cameraNode->addComponent(new bg2e::scene::CameraComponent());
    auto projection = new bg2e::math::OpticalProjection();
    projection->setFocalLength(35.0f);
    projection->setFrameSize(35.0f);
    projection->setFar(1000.0f);
    cameraNode->camera()->setProjection(projection);
    
    auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
    cameraRotation->addComponent(new bg2e::scene::TransformComponent());
    auto cameraRotationComponent = new bg2e::scene::OrbitCameraComponent();
    _orbitCamera = cameraRotationComponent;
    cameraRotationComponent->setMaxX(std::numeric_limits<float>::max());
    cameraRotationComponent->setMaxY(std::numeric_limits<float>::max());
    cameraRotationComponent->setMaxZ(std::numeric_limits<float>::max());
    cameraRotationComponent->setMinX(-std::numeric_limits<float>::max());
    cameraRotationComponent->setMinY(-std::numeric_limits<float>::max());
    cameraRotationComponent->setMinZ(-std::numeric_limits<float>::max());
    cameraRotationComponent->setDistance(2.0f);
    cameraRotationComponent->setMaxDistance(300.0f);
    cameraRotationComponent->setInitialDistance(2.0f);
    cameraRotationComponent->setWheelSpeed(2.0f);
    cameraRotationComponent->setPanSpeed(0.5f);
    cameraRotationComponent->setRotation(glm::vec2{ 13.0f, 10.0f });
    
    cameraRotation->addComponent(cameraRotationComponent);
    cameraRotation->addChild(cameraNode);
    sceneRoot->addChild(cameraRotation);

    cameraNode->addComponent(new bg2e::manipulation::GizmoComponent(_engine));

    auto environmentNode = new bg2e::scene::Node("Environment");
    sceneRoot->addChild(environmentNode);
    environmentNode->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "mirrored_hall_4k.hdr"));
    environmentNode->addComponent(new bg2e::manipulation::GizmoComponent(_engine));
    _environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(environmentNode->environment()->shared_from_this());
    _environmentNode = std::static_pointer_cast<bg2e::scene::Node>(environmentNode->shared_from_this());

    _lightsNode = std::make_shared<bg2e::scene::Node>("Lights");
    _lightsNode->addComponent(new bg2e::scene::TransformComponent());
    environmentNode->addChild(_lightsNode);

    auto directionalLight = createLightNode(
        bg2e::base::Light::TypeDirectional,
        225.0f, 56.27f, 3.33f,
        bg2e::base::Color::White(),
        "Directional Light"
    );

    // Optimize the directional light to allow a quick change to spot light
    auto polarTrx = directionalLight->getComponent<bg2e::scene::PolarTransformControllerComponent>();
    if (polarTrx)
    {
        polarTrx->setEulerX(58.27f);
        polarTrx->setEulerY(40.0f);
    }
    directionalLight->light()->light().setIntensity(5.25f);
    directionalLight->light()->light().setSpotAngle(30.2f);
    directionalLight->light()->light().setSpotCutoff(24.4);
    _lightsNode->addChild(directionalLight);

    auto pointLight = createLightNode(
        bg2e::base::Light::TypeOmni,
        45.0f, 30.0f, 2.0,
        bg2e::base::Color(0.3f, 0.5f, 0.9f, 1.0f),
        "Point Light"
    );
    _lightsNode->addChild(pointLight);

    // Target node: the node where the loaded model is placed
    _targetNode = std::make_shared<bg2e::scene::Node>("Target Node");
    sceneRoot->addChild(_targetNode);

    auto floor = createFloorNode();
    environmentNode->addChild(floor);
    
    _sceneRoot = sceneRoot;

    // Restore environment settings
    restoreEnvironmentSettings();

    return _sceneRoot;
}

void StageScene::loadModel(const std::filesystem::path& path)
{
    close();

    auto modelDrawable = bg2e::db::loadDrawableBg2(path, _engine);
    _targetDrawable = std::shared_ptr<bg2e::scene::Drawable>(modelDrawable);
    auto modelNode = new bg2e::scene::Node("Target model");
    modelNode->addComponent(new bg2e::scene::DrawableComponent(modelDrawable));
    modelNode->addComponent(new bg2e::manipulation::SelectableComponent());

    _targetNode->addChild(modelNode);

    _document->setPath(path);
    _document->setUnsavedChanges(false);
}

void StageScene::saveModel(const std::filesystem::path& path)
{
    auto drw = targetDrawable();
    if (drw)
    {
        bg2e::db::storeDrawableBg2(path, drw);
        _document->setPath(path);
    }
    _document->setUnsavedChanges(false);
}

void StageScene::close()
{
    _appDelegate->selectionManager()->deselect();
    _targetDrawables.clear();
    _targetDrawable.reset();
    _targetNode->clearChildren();
    _targetScene.reset();
    _targetNames.clear();
    _document->setStatus("", false);
}


void StageScene::importModel(const std::filesystem::path& path)
{
    // Get file extension
    std::string extension = path.extension().string();
    std::transform(
        extension.cbegin(),
        extension.cend(),
        extension.begin(),
        [](unsigned char c) { return std::tolower(c); }
    );
    if (extension == ".obj")
    {
        importObj(path);
    }
    else if (extension == ".glb" || extension == ".gltf")
    {
        importGltf(path);
    }
    else
    {
        throw std::runtime_error("Unsupported extension");
    }
    _document->setUnsavedChanges(true);
}

void StageScene::importObj(const std::filesystem::path& path)
{
    close();

    auto objDrawable = bg2e::db::loadDrawableObj(path, _engine);
    _targetDrawable = std::shared_ptr<bg2e::scene::Drawable>(objDrawable);
    auto modelNode = new bg2e::scene::Node("Target model");
    modelNode->addComponent(new bg2e::scene::DrawableComponent(objDrawable));
    modelNode->addComponent(new bg2e::manipulation::SelectableComponent());

    _targetNode->addChild(modelNode);

    _document->setStatus("", true);
}

void StageScene::importGltf(const std::filesystem::path& path)
{
    close();

    auto gltfScene = bg2e::db::loadGltf(path, _engine);

    if (gltfScene != nullptr)
    {
        bg2e::scene::FindNodeComponentVisitor<bg2e::scene::DrawableComponent> findDrawables;
        auto drawableNodes = findDrawables.find(gltfScene);
        for (auto& drawableNodeWp : drawableNodes)
        {
            auto drawableNode = drawableNodeWp.lock();
            if (!drawableNode) continue;
            auto drawable = drawableNode->getComponent<bg2e::scene::DrawableComponent>()->drawable();
            _targetDrawables.push_back(drawable);
            _targetNames.push_back(drawableNode->name());
        }
        _targetScene = std::shared_ptr<bg2e::scene::Node>(gltfScene);

        selectTargetNode(0);
    }

    _document->setStatus("", true);
}

void StageScene::selectTargetNode(uint32_t index)
{
    if (_targetDrawables.at(index).get() != nullptr)
    {
        _selectedTargetNode = index;
        _targetNode->clearChildren();

        // Get the DrawableComponent from the selected node and add it to a new empty node
        auto drawable = _targetDrawables[index];
        auto node = std::make_shared<bg2e::scene::Node>("Target Node");
        node->addComponent(new bg2e::scene::DrawableComponent(drawable));
        node->addComponent(new bg2e::manipulation::SelectableComponent());
        _targetNode->addChild(node);
        _targetDrawable = drawable;
    }

    _appDelegate->selectionManager()->deselect();
}

std::shared_ptr<bg2e::scene::Drawable> StageScene::targetDrawable()
{
    return _targetDrawable;
}


void StageScene::cleanup()
{
    _targetDrawable.reset();
    _targetDrawables.clear();
    _targetScene.reset();
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
                    { "bg2e 3D model", "bg2,vwglb" }
                });
                auto filePath = fd.saveFile();
                if (filePath.empty())
                {
                    return false;
                }

                _document->setPath(filePath);
            }
            saveModel(_document->path());
            return true;
        }
        else if (response == 2)
        {
            return true;
        }
    }
    return true;
}

void StageScene::iterateLights(std::function<void(std::shared_ptr<bg2e::scene::LightComponent> lightComp)> cb)
{
    for (auto l : _sceneRoot->scene()->lightComponents())
    {
        cb(l);
    }
}

void StageScene::saveEnvironmentSettings()
{
    auto settingsPath = bg2e::base::PlatformTools::settingsPath();
    auto environmentSettingsPath = settingsPath / "env.json";

    saveEnvironmentSettings(environmentSettingsPath);
}

void StageScene::saveEnvironmentSettings(const std::filesystem::path& path)
{
    bg2e::db::saveScene(_environmentNode.get(), path);
}

void StageScene::restoreEnvironmentSettings()
{
    auto settingsPath = bg2e::base::PlatformTools::settingsPath();
    auto environmentSettingsPath = settingsPath / "env.json";

    restoreEnvironmentSettings(environmentSettingsPath);
}

void StageScene::restoreEnvironmentSettings(const std::filesystem::path& path)
{
    if (_restoringEnvironment) return;
    _restoringEnvironment = true;

    auto newScene = bg2e::db::loadScene(path, *_engine);
    if (!newScene)
    {
        return;
    }

    auto findEnvironment = std::make_shared<bg2e::scene::FindNodeComponentVisitor<bg2e::scene::EnvironmentComponent>>();
    auto environmentNode = findEnvironment->find(newScene->rootNode());

    auto findLights = std::make_shared<bg2e::scene::FindNodeByProperties>();
    findLights->byName("Lights");
    auto lightNodes = findLights->find(newScene->rootNode());

    auto findFloor = std::make_shared<bg2e::scene::FindNodeByProperties>();
    findFloor->byName("Floor");
    auto floorNodes = findFloor->find(newScene->rootNode());

    if (!environmentNode.empty() && lightNodes.size() == 1 && floorNodes.size() == 1)
    {
        _restoreToken = std::make_shared<bg2e::app::SafeUpdateToken>();
        bg2e::app::MainLoop::current()->safeUpdateScene(
            [this, environmentNode, newScene, lightNodes, floorNodes]()
            {
                auto envNode = environmentNode[0].lock();
                if (!envNode) { _restoringEnvironment = false; return; }
                _sceneRoot->removeChild(_environmentNode);
                _environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(envNode->environment()->shared_from_this());
                _environmentNode = std::static_pointer_cast<bg2e::scene::Node>(newScene->rootNode()->shared_from_this());
                _lightsNode = lightNodes[0];
                _floorNode = floorNodes[0];
                _sceneRoot->addChild(_environmentNode);

                if (_floorNode && !_floorNode->drawable())
                {
                    auto floorDrawable = std::make_shared<bg2e::scene::Drawable>();
                    auto floorGeo = bg2e::geo::createPlane(100.0f, 100.0f);
                    floorDrawable->setMesh(floorGeo);
                    floorDrawable->load(_engine);
                    _floorNode->addComponent(new bg2e::scene::DrawableComponent(floorDrawable));
                }

                if (_floorNode && _floorNode->transform())
                {
                    _floorHeight = _floorNode->transform()->matrix()[3][1];
                }
                _showFloor = _floorNode ? _floorNode->enabled() : true;

                _sceneRoot->scene()->updateAll();
                _restoringEnvironment = false;

                for (auto l : _sceneRoot->scene()->lightComponents())
                {
                    auto lightNode = l->ownerNode();
                    if (!lightNode->getComponent<bg2e::manipulation::GizmoComponent>())
                    {
                        lightNode->addComponent(new bg2e::manipulation::GizmoComponent(_engine));
                    }
                }
            },
            _restoreToken
        );
    }
    else
    {
        _restoringEnvironment = false;
        if (environmentNode.empty())
        {
            std::cerr << "WARN: Could not restore environment: EnvironmentComponent not found" << std::endl;
        }
        else if (lightNodes.size() != 1)
        {
            std::cerr << "WARN: Could not restore environment: Lights node not found" << std::endl;
        }
        else if (floorNodes.size() != 1)
        {
            std::cerr << "WARN: Could not restore environment: Floor node not found" << std::endl;
        }
        bg2e::app::MessageBox msg;
        msg.showError("Invalid environment", "The specified file does not appear to be a valid environment file");
    }
}

void StageScene::addLight()
{
    auto azimuth = std::rand() % 360;
    auto elevation = std::rand() % 180 - 90;
    auto distance = 2.0f;
    auto l = createLightNode(
        bg2e::base::Light::TypeOmni,
        static_cast<float>(azimuth), static_cast<float>(elevation)
    );
    _lightsNode->addChild(l);
    _sceneRoot->scene()->updateLights();
}

void StageScene::removeLight(uint32_t index)
{
    if (_sceneRoot->scene()->lightComponents().size() > index)
    {
        auto lightComp = _sceneRoot->scene()->lightComponents()[index];
        if (lightComp)
        {
            auto node = lightComp->ownerNode();
            _lightsNode->removeChild(node->shared_from_this());
        }
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::createLightNode(
   bg2e::base::Light::LightType type,
   float azimuth,
   float elevation,
   float distance,
   const bg2e::base::Color& color,
   const std::string & name
) {
    using namespace bg2e::scene;
    auto lightName = name;
    if (lightName == "")
    {
        auto lightNumber = lights().size();
        lightName = "Light " + std::to_string(++lightNumber);
    }

    auto node = std::make_shared<bg2e::scene::Node>(lightName);

    node->addComponent(new bg2e::scene::LightComponent());
    node->addComponent(new bg2e::scene::TransformComponent());
    auto polarController = new bg2e::scene::PolarTransformControllerComponent();
    polarController->setAzimuth(azimuth);
    polarController->setElevation(elevation);
    polarController->setDistance(distance);
    polarController->setEulerX(45.0f);
    polarController->setEulerY(45.0f);
    polarController->setPriority(2);
    node->addComponent(polarController);

    node->addComponent(new bg2e::manipulation::GizmoComponent(_engine));

    node->light()->light().setIntensity(2.0f);
    node->light()->light().setType(type);

    return node;
}

void StageScene::setFlorHeight(float h)
{
    _floorHeight = h;
    if (_floorNode && _floorNode->transform())
    {
        _floorNode->transform()->setTranslation(0.0f, _floorHeight, 0.0f);
    }
}

void StageScene::showFloor(bool show)
{
    _showFloor = show;
    if (_floorNode)
    {
        _floorNode->setEnabled(_showFloor);
    }
}

std::shared_ptr<bg2e::scene::Node> StageScene::createFloorNode()
{
    _floorNode = std::make_shared<bg2e::scene::Node>("Floor");
    _floorNode->setEnabled(_showFloor);
    _floorNode->addComponent(new bg2e::manipulation::SelectableComponent());
    _floorNode->addComponent(new bg2e::scene::TransformComponent());
    _floorNode->transform()->setTranslation(0.0f, _floorHeight, 0.0f);

    auto floorGeo = bg2e::geo::createPlane(100.0f, 100.0f);
    auto drawable = std::make_shared<bg2e::scene::Drawable>();
    drawable->setMesh(floorGeo);
    drawable->load(_engine);
    auto dc = new bg2e::scene::DrawableComponent(drawable);
    _floorNode->addComponent(dc);

    return _floorNode;
}

bg2e::scene::CameraComponent * StageScene::cameraComponent()
{
    auto orbitNode = _orbitCamera->ownerNode();
    for (auto & child : orbitNode->children()) {
        auto cam = child->getComponent<bg2e::scene::CameraComponent>();
        if (cam) return cam;
    }
    return nullptr;
}

bg2e::scene::CameraComponent * StageScene::cameraComponent() const
{
    auto orbitNode = _orbitCamera->ownerNode();
    for (auto & child : orbitNode->children()) {
        auto cam = child->getComponent<bg2e::scene::CameraComponent>();
        if (cam) return cam;
    }
    return nullptr;
}

