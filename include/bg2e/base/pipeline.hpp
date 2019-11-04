#ifndef _bg2e_base_pipeline_hpp_
#define _bg2e_base_pipeline_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/poly_list.hpp>
#include <bg2e/base/shader.hpp>
#include <bg2e/math/matrix.hpp>
#include <bg2e/base/material.hpp>


namespace bg2e {
namespace base {

	class Pipeline : public ReferencedPointer {
	public:
		Pipeline(bgfx::ViewId viewId);

		inline bgfx::ViewId viewId() const { return _viewId; }

		inline void setProjection(const math::float4x4 & p) { _projection = p; }
		inline void setView(const math::float4x4 & v) { _view = v; }
		inline math::float4x4 & projection() { return _projection; }
		inline const math::float4x4 & projection() const { return _projection; }
		inline math::float4x4 & view() { return _view; }
		inline const math::float4x4 & view() const { return _view; }

		inline void setClearColor(const math::color & c) { _clearColor = c; }
		inline const math::color & clearColor() const { return _clearColor; }
		inline math::color & clearColor() { return _clearColor; }

		inline void setClearDepth(float d) { _clearDepth = d; }
		inline float clearDepth() const { return _clearDepth; }

		inline void setClearBuffers(bool color, bool depth) {
			_clearFlags =	(color ? BGFX_CLEAR_COLOR : 0) |
							(depth ? BGFX_CLEAR_DEPTH : 0);
		}
		inline bool clearColorEnabled() const { return (_clearFlags & BGFX_CLEAR_COLOR) != 0; }
		inline bool clearDepthEnabled() const { return (_clearFlags & BGFX_CLEAR_DEPTH) != 0; }

		inline void setShader(Shader * s) {
			_shader = s;
			if (!_shader->isCreated()) {
				_shader->create();
			}
		}
		inline Shader * shader() { return _shader.getPtr(); }
		inline const Shader * shader() const { return _shader.getPtr(); }

		void beginDraw();
		void draw(PolyList * plist, Material * material, const math::float4x4 & modelMatrix);

	protected:
		virtual ~Pipeline();

		bgfx::ViewId _viewId;

		math::color _clearColor = math::color(0.0f, 0.0f, 0.0f, 1.0f);
		float _clearDepth = 1.0f;
		uint16_t _clearFlags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;

		math::float4x4 _projection = math::float4x4::Identity();
		math::float4x4 _view = math::float4x4::Identity();

		ptr<Shader> _shader;
	};
}
}

#endif
