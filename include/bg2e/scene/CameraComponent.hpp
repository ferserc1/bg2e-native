//
//  CameraComponent.hpp
#pragma once

#include <bg2e/base/Camera.hpp>
#include <bg2e/scene/Component.hpp>

namespace bg2e {
namespace scene {

class BG2E_API CameraComponent : public Component {
public:
    static std::string componentName() { return "Camera"; }
    
    CameraComponent();
    virtual ~CameraComponent();
    
    const base::Camera& camera() const { return _camera; }
    base::Camera& camera() { return _camera; }
    
    math::Projection * projection() { return _camera.projection(); }
    template <class T>
    T* projection() { return _camera.projection<T>(); }
    void setProjection(math::Projection * proj) { _camera.setProjection(proj); }
    void setProjection(std::shared_ptr<math::Projection> proj) { _camera.setProjection(proj); }
    const glm::mat4& projectionMatrix() const { return _camera.projectionMatrix(); }
    
    void resizeViewport(const math::Viewport& vp) override;
    void update(float delta) override;
    
protected:
    base::Camera _camera;
};


}
}
