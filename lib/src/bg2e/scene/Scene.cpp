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

#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/FindNodeVisitor.hpp>
#include <bg2e/scene/FindCameraVisitor.hpp>
#include <bg2e/scene/FindNodeComponentVisitor.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/base/Log.hpp>

namespace bg2e::scene {

std::shared_ptr<Scene> Scene::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{
    if (!jsonData || !jsonData->isObject())
    {
        return nullptr;
    }

    auto& obj = jsonData->objectValue();

    // Read and log version
    if (obj.count("version") && obj["version"]->isObject())
    {
        auto& version = obj["version"]->objectValue();
        int major = version.count("major") ? static_cast<int>(version["major"]->numberValue(0)) : 0;
        int minor = version.count("minor") ? static_cast<int>(version["minor"]->numberValue(0)) : 0;
        int rev = version.count("rev") ? static_cast<int>(version["rev"]->numberValue(0)) : 0;
        bg2e_log_debug << "Loading scene file version " << major << "." << minor << "." << rev << bg2e_log_end;
    }

    // Create root node
    auto sceneRoot = std::make_shared<Node>("scene root");

    // Deserialize nodes from scene array
    if (obj.count("scene") && obj["scene"]->isList())
    {
        auto& sceneList = obj["scene"]->listValue();
        for (auto& nodeData : sceneList)
        {
            auto node = std::make_shared<Node>();
            node->deserialize(nodeData, basePath);
            sceneRoot->addChild(node);
        }
    }

    auto scene = std::make_shared<Scene>();
    scene->setSceneRoot(sceneRoot);
    return scene;
}

void Scene::setSceneRoot(std::shared_ptr<Node> sceneRoot)
{
    if (sceneRoot->_scene != nullptr)
    {
        throw std::runtime_error("Scene::setSceneRoot(): The specified scene root is already added to an scene.");
    }
    
    _sceneRoot = sceneRoot;
    _mainEnvironment = nullptr;
    _mainCameraNode = nullptr;
    _sceneRoot->_scene = this;
    updateLights();
}

void Scene::setMainCamera(CameraComponent * camera)
{
    if (!camera)
    {
        throw std::runtime_error("Scene::setMainCamera() - Invalid camera: nullptr");
    }
    FindNodeVisitor findCamera;
    if (findCamera.belongsToScene(camera->ownerNode(), _sceneRoot.get()))
    {
        // The camera belongs to the scene: set main camera
        _mainCameraNode = camera->ownerNode();
    }
    else{
        throw std::runtime_error("Scene::setMainCamera() - The camera node does not belong to this scene");
    }
}
    
CameraComponent * Scene::mainCamera()
{
    if (!_mainCameraNode)
    {
        FindCameraVisitor findCamera;
        findCamera.findCameras(_sceneRoot.get());
        auto camList = findCamera.cameras();
        if (camList.size() > 0)
        {
            if (auto cam = camList[0].lock())
            {
                _mainCameraNode = cam->ownerNode();
            }
        }
        
        if (!_mainCameraNode)
        {
            throw std::runtime_error("Scenes::mainCamera() - No cameras found in the scene");
        }
    }
    return _mainCameraNode->camera();
}

EnvironmentComponent * Scene::mainEnvironment()
{
    if (!_mainEnvironment)
    {
        FindNodeComponentVisitor<EnvironmentComponent> findEnvironment;
        auto envs = findEnvironment.find(_sceneRoot.get());
        
        // Get the first environment found in the scene.
        // TODO: improve this
        // It should be possible to choose which environment you want to use if there are several in the scene
        if (envs.size() > 0)
        {
            if (auto envNode = envs.begin()->lock())
            {
                _mainEnvironment = envNode.get();
            }
        }
    }
    return _mainEnvironment != nullptr ? _mainEnvironment->environment() : nullptr;
}

void Scene::updateLights()
{
    _lightComponents.clear();
    _lights.clear();
    FindNodeComponentVisitor<LightComponent> findLights;
    auto lightNodes = findLights.find(_sceneRoot.get());
    for (auto& l : lightNodes)
    {
        auto node = l.lock();
        if (!node) continue;
        auto lc = std::dynamic_pointer_cast<LightComponent>(node->light()->shared_from_this());
        if (lc.get() != nullptr)
        {
            _lightComponents.push_back(lc);
            _lights.push_back(&node->light()->light());
        }
    }
    _lightsChanged = true;
}

void Scene::willResize()
{

}

void Scene::didResize()
{

}

void Scene::willUpdate()
{

}

void Scene::didUpdate()
{

}

void Scene::willDraw()
{
}

void Scene::didDraw()
{
    _lightsChanged = false;
}


}
