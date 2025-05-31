//
//  Scene.cpp

#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/FindNodeVisitor.hpp>
#include <bg2e/scene/FindCameraVisitor.hpp>
#include <bg2e/scene/FindNodeComponentVisitor.hpp>

namespace bg2e::scene {

void Scene::setSceneRoot(std::shared_ptr<Node> scene)
{
    _sceneRoot = scene;
    _mainEnvironment = nullptr;
    _mainCameraNode = nullptr;
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
        if (findCamera.cameras().size() > 0)
        {
            _mainCameraNode = findCamera.cameras()[0]->ownerNode();
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
        _mainEnvironment = *envs.begin();
    }
    return _mainEnvironment->environment();
}
}
