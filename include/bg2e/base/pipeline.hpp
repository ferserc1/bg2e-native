#ifndef _bg2e_base_pipeline_hpp_
#define _bg2e_base_pipeline_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/poly_list.hpp>
#include <bg2e/base/shader.hpp>
#include <bg2e/base/matrix_stack.hpp>
#include <bg2e/base/material.hpp>


namespace bg2e {
namespace base {

	class Pipeline : public ReferencedPointer {
	public:
		Pipeline(bgfx::ViewId viewId);

		inline bgfx::ViewId viewId() const { return _viewId; }

		inline MatrixStack & projection() { return _projectionMatrixStack; }
		inline const MatrixStack & projection() const { return _projectionMatrixStack; }
		inline MatrixStack & view() { return _viewMatrixStack; }
		inline const MatrixStack & view() const { return _viewMatrixStack; }
		inline MatrixStack & model() { return _modelMatrixStack; }
		inline const MatrixStack & model() const { return _modelMatrixStack; }


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
		void draw(PolyList * plist, Material * material);

	protected:
		virtual ~Pipeline();

		bgfx::ViewId _viewId;

		math::color _clearColor = math::color(0.0f, 0.0f, 0.0f, 1.0f);
		float _clearDepth = 1.0f;
		uint16_t _clearFlags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;

		ptr<Shader> _shader;

		MatrixStack _projectionMatrixStack;
		MatrixStack _viewMatrixStack;
		MatrixStack _modelMatrixStack;
	};
}
}

#endif
