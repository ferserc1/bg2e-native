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
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/base/Log.hpp>
#include <stdexcept>
#include <utility>

namespace bg2e::scene {

Scene::~Scene()
{
    if (_sceneRoot && _sceneRoot->_scene == this)
    {
        _sceneRoot->_scene = nullptr;
    }
}

std::shared_ptr<Scene> Scene::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, SceneLoadProgress* progress)
{
    json::ObjectReader reader(jsonData);
    if (!reader.isValid() || !reader.getArray("scene"))
    {
        throw std::invalid_argument("Scene representation requires a scene array");
    }

    // Read and log version
    if (auto version = reader.getObject("version"))
    {
        int major = version->getInteger<int>("major").value_or(0);
        int minor = version->getInteger<int>("minor").value_or(0);
        int rev = version->getInteger<int>("rev").value_or(0);
        bg2e_log_debug << "Loading scene file version " << major << "." << minor << "." << rev << bg2e_log_end;
    }

    // Create root node
    auto sceneRoot = std::make_shared<Node>("scene root");

    // Deserialize nodes from scene array
    if (auto sceneList = reader.getArray("scene"))
    {
        for (std::size_t i = 0; i < sceneList->size(); ++i)
        {
            auto nodeData = sceneList->getObject(i);
            if (!nodeData)
            {
                throw std::invalid_argument("Invalid scene node representation");
            }
            auto node = std::make_shared<Node>();
            node->deserialize(nodeData->node(), basePath, progress);
            sceneRoot->addChild(node);
        }
    }

    auto scene = std::make_shared<Scene>();
    scene->setSceneRoot(sceneRoot);
    return scene;
}

std::shared_ptr<Scene> Scene::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath,
    draw::Engine& engine, SceneLoadProgress* progress)
{
    auto result = deserialize(data, basePath, progress);
    result->rootNode()->loadResources(engine, progress);
    return result;
}

std::shared_ptr<Scene> Scene::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, render::Engine& engine, SceneLoadProgress* progress)
{
    json::ObjectReader reader(jsonData);
    if (!reader.isValid())
    {
        return nullptr;
    }

    // Read and log version
    if (auto version = reader.getObject("version"))
    {
        int major = version->getInteger<int>("major").value_or(0);
        int minor = version->getInteger<int>("minor").value_or(0);
        int rev = version->getInteger<int>("rev").value_or(0);
        bg2e_log_debug << "Loading scene file version " << major << "." << minor << "." << rev << bg2e_log_end;
    }

    // Create root node
    auto sceneRoot = std::make_shared<Node>("scene root");

    // Deserialize nodes from scene array
    if (auto sceneList = reader.getArray("scene"))
    {
        for (std::size_t i = 0; i < sceneList->size(); ++i)
        {
            auto nodeData = sceneList->getObject(i);
            if (!nodeData)
            {
                bg2e_log_warning << "Skipping invalid scene node at index " << i << bg2e_log_end;
                continue;
            }
            auto node = std::make_shared<Node>();
            node->deserialize(nodeData->node(), basePath, engine, progress);
            sceneRoot->addChild(node);
        }
    }

    auto scene = std::make_shared<Scene>();
    scene->setSceneRoot(sceneRoot);
    return scene;
}

bool Scene::sceneHasCamera(Node* rootNode)
{
    if (!rootNode)
    {
        return false;
    }
    FindCameraVisitor findCamera;
    findCamera.findCameras(rootNode);
    return !findCamera.cameras().empty();
}

bool Scene::validateScene(Node* rootNode)
{
    // Add future checks here as additional independent calls, for example:
    //   return sceneHasCamera(rootNode) && sceneHasEnvironment(rootNode);
    return sceneHasCamera(rootNode);
}

void Scene::setSceneRoot(std::shared_ptr<Node> sceneRoot)
{
    if (!sceneRoot)
    {
        throw std::invalid_argument("Scene::setSceneRoot requires a non-null root");
    }
    if (sceneRoot->_scene != nullptr)
    {
        throw std::runtime_error("Scene::setSceneRoot(): The specified scene root is already added to an scene.");
    }

    if (_sceneRoot && _sceneRoot->_scene == this)
    {
        _explicitEnvironment = false;
        _mainEnvironment = nullptr;
        _selectedEnvironmentNode.reset();
        _selectedEnvironment.reset();
        _sceneRoot->_scene = nullptr;
    }
    _sceneRoot = sceneRoot;
    _mainEnvironment = nullptr;
    _mainCameraNode = nullptr;
    _explicitEnvironment = false;
    _selectedEnvironmentNode.reset();
    _selectedEnvironment.reset();
    _sceneRoot->_scene = this;
    updateLights();
}

std::shared_ptr<Node> Scene::releaseSceneRoot()
{
    auto root = std::move(_sceneRoot);
    if (root)
    {
        root->_scene = nullptr;
    }
    _mainCameraNode = nullptr;
    _mainEnvironment = nullptr;
    _lightComponents.clear();
    _explicitEnvironment = false;
    _selectedEnvironmentNode.reset();
    _selectedEnvironment.reset();
    _lights.clear();
    _lightsChanged = true;
    return root;
}

void Scene::setMainCamera(CameraComponent* camera)
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
    else
    {
        throw std::runtime_error("Scene::setMainCamera() - The camera node does not belong to this scene");
    }
}

CameraComponent* Scene::mainCamera()
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

EnvironmentComponent* Scene::mainEnvironment()
{
    if (!_sceneRoot)
    {
        return nullptr;
    }
    if (_explicitEnvironment)
    {
        auto node = _selectedEnvironmentNode.lock();
        auto component = _selectedEnvironment.lock();
        if (!node || !component || component->ownerNode() != node.get() || node->environment() != component.get())
        {
            return nullptr;
        }
        FindNodeVisitor find;
        find.setIgnoreDisabled(true);
        if (node->disabled() || !find.belongsToScene(node.get(), _sceneRoot.get()))
        {
            return nullptr;
        }
        return component.get();
    }
    FindNodeComponentVisitor<EnvironmentComponent> findEnvironment;
    findEnvironment.setIgnoreDisabled(true);
    auto envs = findEnvironment.find(_sceneRoot.get());
    if (envs.size() > 0)
    {
        if (auto envNode = envs.front().lock())
        {
            auto* component = envNode->environment();
            if (component)
            {
                setMainEnvironment(component);
                return component;
            }
        }
    }
    setMainEnvironment(nullptr);
    return nullptr;
}

void Scene::setMainEnvironment(EnvironmentComponent* environment)
{
    std::shared_ptr<Node> selected;
    if (environment)
    {
        auto* node = environment->ownerNode();
        FindNodeVisitor find;
        find.setIgnoreDisabled(true);
        if (!_sceneRoot || !node || node->environment() != environment ||
            !find.belongsToScene(node, _sceneRoot.get()))
        {
            throw std::invalid_argument("The active environment must belong to an enabled scene branch");
        }
        selected = node->shared_from_this();
    }
    else if (!_sceneRoot)
    {
        // An unattached Scene represents implicit renderer defaults until its
        // root is adopted. Do not freeze a default selection before loading.
        resetMainEnvironmentSelection();
        return;
    }
    _explicitEnvironment = _sceneRoot != nullptr;
    _mainEnvironment = nullptr;
    _selectedEnvironmentNode = std::move(selected);
    _selectedEnvironment = environment ?
        std::dynamic_pointer_cast<EnvironmentComponent>(environment->shared_from_this()) : nullptr;
}

void Scene::resetMainEnvironmentSelection()
{
    _explicitEnvironment = false;
    _mainEnvironment = nullptr;
    _selectedEnvironmentNode.reset();
    _selectedEnvironment.reset();
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
        if (!node)
        {
            continue;
        }
        auto lc = std::dynamic_pointer_cast<LightComponent>(node->light()->shared_from_this());
        if (lc.get() != nullptr)
        {
            _lightComponents.push_back(lc);
            _lights.push_back(&node->light()->light());
        }
    }
    _lightsChanged = true;
}

void Scene::updateEnvironment()
{
    if (_explicitEnvironment)
    {
        // Revalidate the weak selected owner; never promote an ignored sibling.
        mainEnvironment();
        return;
    }
    _mainEnvironment = nullptr;
    mainEnvironment();
}

void Scene::updateCamera()
{
    _mainCameraNode = nullptr;
    // Force update main camera
    mainCamera();
}

void Scene::updateAll()
{
    updateLights();
    updateEnvironment();
    updateCamera();
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
