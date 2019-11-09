
#include <bg2e/base/camera.hpp>
#include <bg2e/math/matrix.hpp>
#include <bg2e/math/matrix_strategy.hpp>

namespace bg2e {
namespace base {

	Camera::Camera()
	{
		_projection.perspective(60.0f, 1.0f, 0.3f, 1000.0f);
		_view.identity();
		_viewport = math::viewport(512, 512);
	}

	Camera::~Camera() {

	}
	
	void Camera::setViewport(const math::viewport & vp) {
		_viewport = vp;
		if (_projectionStrategy.valid()) {
			_projectionStrategy->setViewport(_viewport);
			_projectionStrategy->apply();
		}
	}


	void Camera::setProjectionStrategy(ProjectionStrategy * s) {
		_projectionStrategy = s;
		if (_projectionStrategy.valid()) {
			_projectionStrategy->setViewport(_viewport);
			_projectionStrategy->setTarget(reinterpret_cast<math::float4x4*>(&_projection));
			_projectionStrategy->apply();
		}
	}

}
}
