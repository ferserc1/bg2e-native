#ifndef _bg2e_base_projection_strategy_
#define _bg2e_base_projection_strategy_

#include <bg2e/math/matrix_strategy.hpp>
#include <bg2e/base/referenced_pointer.hpp>

#include <bg2e/db/json/value.hpp>

namespace bg2e {
namespace base {

	class ProjectionStrategy : public math::MatrixStrategy, public ReferencedPointer {
	public:

		static ProjectionStrategy* Factory(db::json::Value* jsonData);

		ProjectionStrategy();
		ProjectionStrategy(math::float4x4 * mat);

		virtual ProjectionStrategy * clone() = 0;

		inline void setNear(float n) { _near = n; }
		inline float near() const { return _near; }
		inline void setFar(float f) { _far = f; }
		inline float far() const { return _far; }
		inline void setViewport(const math::viewport & vp) { _viewport = vp; }
		inline math::viewport & viewport() { return _viewport; }
		inline const math::viewport & viewport() const { return _viewport; }

		virtual db::json::Value * serialize() = 0;
		virtual void deserialize(db::json::Value * value) = 0;

	protected:
		virtual ~ProjectionStrategy();

		float _near;
		float _far;
		math::viewport _viewport;
	};

	class PerspectiveProjectionStrategy : public ProjectionStrategy {
	public:
		PerspectiveProjectionStrategy();
		PerspectiveProjectionStrategy(math::float4x4 *);

		virtual ProjectionStrategy* clone();

		inline void setFov(float f) { _fov = f; }
		inline float fov() const { return _fov; }

		virtual void apply();

		virtual db::json::Value * serialize();
		virtual void deserialize(db::json::Value * data);

	protected:
		virtual ~PerspectiveProjectionStrategy();

		float _fov;
	};

	class OpticalProjectionStrategy : public ProjectionStrategy {
	public:
		OpticalProjectionStrategy();
		OpticalProjectionStrategy(math::float4x4 * mat);

		virtual ProjectionStrategy * clone();

		inline void setFocalLength(float fl) { _focalLength = fl; }
		inline float focalLength() const { return _focalLength; }
		inline void setFrameSize(float fs) { _frameSize = fs; }
		inline float frameSize() const { return _frameSize; }

		virtual void apply();

		virtual db::json::Value* serialize();

		virtual void deserialize(db::json::Value*);

	protected:
		virtual ~OpticalProjectionStrategy();

		float _focalLength;
		float _frameSize;
	};
}
}

#endif
