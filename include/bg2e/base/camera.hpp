#ifndef _bg2e_base_camera_hpp_
#define _bg2e_base_camera_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/projection_strategy.hpp>

namespace bg2e {
namespace base {

	class Camera {
	public:
		Camera();
		virtual ~Camera();

		inline void operator = (const Camera & other) {
			_projection = other._projection;
			_view = other._view;
			_viewport = other._viewport;
			_projectionStrategy = other._projectionStrategy;
		}

		inline math::float4x4 & projection() { return _projection; }
		inline const math::float4x4& projection() const { return _projection; }
		inline void setProjection(const math::float4x4 & p) { _projection = p; }
		inline math::float4x4 & view() { return _view; }
		inline const math::float4x4 & view() const { return _view; }
		inline void setView(const math::float4x4 & v) { _view = v; }

		inline const math::viewport & viewport() const { return _viewport; }
		void setViewport(const math::viewport & vp);

		void setProjectionStrategy(ProjectionStrategy *);

		template <class T>
		inline T * projectionStrategy() { return dynamic_cast<T*>(_projectionStrategy.getPtr()); }
		inline ProjectionStrategy * projectionStrategy() { return _projectionStrategy.getPtr(); }

	protected:

		math::float4x4 _projection;
		math::float4x4 _view;
		math::viewport _viewport;

		ptr<ProjectionStrategy> _projectionStrategy;
	};

}
}

#endif
