//
//  CameraComponent.cpp
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>

namespace bg2e::scene {

CameraComponent::CameraComponent()
{

}

CameraComponent::~CameraComponent()
{

}

void CameraComponent::resizeViewport(const math::Viewport& vp)
{
    auto proj = _camera.projection();
    if (proj)
    {
        proj->setViewport(vp);
    }
}

void CameraComponent::update(float delta)
{
    _camera.updateProjectionMatrix();
}

void CameraComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{

}

std::shared_ptr<json::JsonNode> CameraComponent::serialize(const std::filesystem::path& basePath)
{
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject & obj = compData->objectValue();
    
    obj["cameraData"] = _camera.serialize();

    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(CameraComponent);

}
