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

		inline math::float4x4 & projection() { return _projection; }
		inline const math::float4x4& projection() const { return _projection; }
		inline math::float4x4 & view() { return _view; }
		inline const math::float4x4 & view() const { return _view; }

		inline const math::viewport & viewport() const { return _viewport; }
		void setViewport(const math::viewport & vp);

		void setProjectionStrategy(ProjectionStrategy *);

		template <class T>
		inline T * projectionStrategy() { return dynamic_cast<T*>(_projectionStrategy.getPtr()); }

	protected:

		math::float4x4 _projection;
		math::float4x4 _view;
		math::viewport _viewport;

		ptr<ProjectionStrategy> _projectionStrategy;
	};

}
}

#endif
