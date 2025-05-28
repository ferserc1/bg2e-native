//
//  CameraComponent.cpp
#include <bg2e/scene/CameraComponent.hpp>

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

}
