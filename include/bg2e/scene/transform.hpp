
#ifndef _bg2e_scene_transform_hpp_
#define _bg2e_scene_transform_hpp_

#include <bg2e/scene/component.hpp>

#include <bg2e/math/matrix.hpp>

namespace bg2e {
namespace scene {

	class Transform : public Component {
	public:
		static math::float4x4 WorldMatrix(Node *);

		Transform();

		virtual Component * clone();

		inline const math::float4x4 & matrix() const { return _matrix; }
		inline math::float4x4 & matrix() { return _matrix; }

		virtual void willDraw(base::MatrixState *);
		virtual void didDraw(base::RenderQueue &, base::MatrixState *);

		virtual void deserialize(bg2e::db::json::Value *, const bg2e::base::path &);
		virtual bg2e::db::json::Value* serialize(const bg2e::base::path &);

	protected:
		virtual ~Transform();

		math::float4x4 _matrix = math::float4x4::Identity();
	};

}
}

#endif
