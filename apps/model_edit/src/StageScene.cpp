//
//  StageScene.cpp

#include "StageScene.hpp"

#include <bg2e.hpp>

StageScene::StageScene(bg2e::render::Engine * engine)
    :_engine { engine }
{

}

std::shared_ptr<bg2e::scene::Node> StageScene::init()
{
    auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
    sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "gothic_manor_01_4k.hdr"));
    _environment = sceneRoot->environment();
    
    auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
    cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.5f, 2.0f ));
    
    cameraNode->addComponent(new bg2e::scene::CameraComponent());
    auto projection = new bg2e::math::OpticalProjection();
    projection->setFocalLength(35.0f);
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
    
    cameraRotation->addComponent(cameraRotationComponent);
    cameraRotation->addChild(cameraNode);
    sceneRoot->addChild(cameraRotation);
    
    auto light1 = new bg2e::scene::Node("Light 1");
    light1->addComponent(new bg2e::scene::LightComponent());
    light1->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-50, 50, 50 } )));
    light1->light()->light().setIntensity(300.0f);
    sceneRoot->addChild(light1);
    
    // Target node: the node where the loaded model is placed
    _targetNode = std::make_shared<bg2e::scene::Node>("Target Node");
    sceneRoot->addChild(_targetNode);
    
    // Sample model
    auto samplePath = bg2e::base::PlatformTools::assetPath();
    samplePath.append("armchair.bg2");
    loadModel(samplePath);
    
    _sceneRoot = sceneRoot;
    return _sceneRoot;
}

void StageScene::loadModel(const std::filesystem::path& path)
{
    auto modelDrawable = bg2e::db::loadDrawableBg2(path, _engine);
    auto modelNode = new bg2e::scene::Node("Target model");
    modelNode->addComponent(new bg2e::scene::DrawableComponent(modelDrawable));
    
    _targetNode->clearChildren();
    _targetNode->addChild(modelNode);
}

bg2e::scene::Drawable* StageScene::targetDrawable()
{
    bg2e::scene::Drawable * result = nullptr;
    if (_targetNode.get() && _targetNode->children().size() > 0)
    {
        auto drawableComponent = _targetNode->children()[0]->drawable();
        result = drawableComponent ?
            dynamic_cast<bg2e::scene::Drawable*>(drawableComponent->drawable().get())
            : nullptr;
    }
    return result;
}
