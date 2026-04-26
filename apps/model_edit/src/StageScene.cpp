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
#include <string>

StageScene::StageScene(bg2e::render::Engine * engine, AppDelegate * appDelegate)
    :_engine { engine }
    ,_appDelegate { appDelegate }
    ,_document(std::make_unique<Document>(appDelegate) )
{
}

std::shared_ptr<bg2e::scene::Node> StageScene::init()
{
    auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
    sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "mirrored_hall_4k.hdr"));
    _environment = sceneRoot->environment();
    
    auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
    cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.0f, 2.0f ));
    
    cameraNode->addComponent(new bg2e::scene::CameraComponent());
    auto projection = new bg2e::math::OpticalProjection();
    projection->setFocalLength(50.0f);
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
    
    cameraRotation->addComponent(cameraRotationComponent);
    cameraRotation->addChild(cameraNode);
    sceneRoot->addChild(cameraRotation);

    auto sphereRadius = 0.02f;
    auto sphereMesh = std::shared_ptr<bg2e::geo::Mesh>(bg2e::geo::createSphere(sphereRadius, 10, 10));
    auto sphereDrawable = std::make_shared<bg2e::scene::Drawable>();
    sphereDrawable->setMesh(sphereMesh);
    sphereDrawable->load(_engine);
    sphereDrawable->setRayTracingEnabled(false);

    auto light1 = new bg2e::scene::Node("Light 1");
    light1->addComponent(new bg2e::scene::DrawableComponent(sphereDrawable));
    light1->addComponent(new bg2e::scene::LightComponent());
    light1->addComponent(new bg2e::scene::TransformComponent());
    auto polarController = new bg2e::scene::PolarTransformControllerComponent();
    polarController->setDistance(6.0f);
    polarController->setEulerX(12.0f);
    polarController->setEulerY(45.0f);
    light1->addComponent(polarController);
    light1->transform()->rotate(0.2f, 0.0f, 1.0f, 0.0f);
    light1->transform()->rotate(0.6f, 1.0f, 0.0f, 0.0f);
    light1->light()->light().setIntensity(2.0f);
    light1->light()->light().setType(bg2e::base::Light::TypeDirectional);
    sceneRoot->addChild(light1);

    auto light2 = new bg2e::scene::Node("Light 2");
    light2->addComponent(new bg2e::scene::DrawableComponent(sphereDrawable));
    light2->addComponent(new bg2e::scene::LightComponent());
    light2->addComponent(new bg2e::scene::TransformComponent());
    polarController = new bg2e::scene::PolarTransformControllerComponent();
    polarController->setAzimuth(45.0f);
    polarController->setElevation(-10.0f);
    polarController->setDistance(2.0f);
    light2->addComponent(polarController);
    light2->light()->light().setType(bg2e::base::Light::TypeOmni);
    light2->light()->light().setIntensity(20.0f);
    light2->light()->light().setColor(bg2e::base::Color(0.3f, 0.5f, 0.9f, 1.0f));

    sceneRoot->addChild(light2);

    
    // Target node: the node where the loaded model is placed
    _targetNode = std::make_shared<bg2e::scene::Node>("Target Node");
    sceneRoot->addChild(_targetNode);
    
    _sceneRoot = sceneRoot;

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
        for (auto drawableNode : drawableNodes)
        {
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