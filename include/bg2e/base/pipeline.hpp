#ifndef _bg2e_base_pipeline_hpp_
#define _bg2e_base_pipeline_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/poly_list.hpp>
#include <bg2e/base/shader.hpp>
#include <bg2e/math/matrix.hpp>
#include <bg2e/base/material.hpp>
#include <bg2e/base/matrix_state.hpp>


namespace bg2e {
namespace base {

    enum DepthTest {
        kDepthTestLess               = BGFX_STATE_DEPTH_TEST_LESS,
        kDepthTestLessOrEqual        = BGFX_STATE_DEPTH_TEST_LEQUAL,
        kDepthTestEqual              = BGFX_STATE_DEPTH_TEST_EQUAL,
        kDepthTestGreaterOrEqual     = BGFX_STATE_DEPTH_TEST_GEQUAL,
        kDepthTestGreater            = BGFX_STATE_DEPTH_TEST_GREATER,
        kDepthTestNotEqual           = BGFX_STATE_DEPTH_TEST_NOTEQUAL,
        kDepthTestNever              = BGFX_STATE_DEPTH_TEST_NEVER,
        kDepthTestAlways             = BGFX_STATE_DEPTH_TEST_ALWAYS
    };

    enum BlendFunction {
        kBlendFunctionZero               = BGFX_STATE_BLEND_ZERO,
        kBlendFunctionOne                = BGFX_STATE_BLEND_ONE,
        kBlendFunctionSrcColor           = BGFX_STATE_BLEND_SRC_COLOR,
        kBlendFunctionOneMinusSrcColor   = BGFX_STATE_BLEND_INV_SRC_COLOR,
        kBlendFunctionSrcAlpha           = BGFX_STATE_BLEND_SRC_ALPHA,
        kBlendFunctionOneMinusSrcAlpha   = BGFX_STATE_BLEND_INV_SRC_ALPHA,
        kBlendFunctionDstAlpha           = BGFX_STATE_BLEND_DST_ALPHA,
        kBlendFunctionOneMinusDstAlpha   = BGFX_STATE_BLEND_INV_DST_ALPHA,
        kBlendFunctionDstColor           = BGFX_STATE_BLEND_DST_COLOR,
        kBlendFunctionOneMinusDstColor   = BGFX_STATE_BLEND_INV_DST_COLOR,
        kBlendFunctionSrcAlphaSaturate   = BGFX_STATE_BLEND_SRC_ALPHA_SAT,
        kBlendFunctionFactor             = BGFX_STATE_BLEND_FACTOR,
        kBlendFunctionOneMinusFactor     = BGFX_STATE_BLEND_INV_FACTOR,
        kBlendFunctionDisabled           = 0
    };

    enum BlendEquation {
        kBlendEquationAdd        = BGFX_STATE_BLEND_EQUATION_ADD,
        kBlendEquationSub        = BGFX_STATE_BLEND_EQUATION_SUB,
        kBlendEquationReverseSub = BGFX_STATE_BLEND_EQUATION_REVSUB,
        kBlendEquationMin        = BGFX_STATE_BLEND_EQUATION_MIN,
        kBlendEquationMax        = BGFX_STATE_BLEND_EQUATION_MAX,
        kBlendEquationDisabled   = 0
    };

	class Pipeline : public ReferencedPointer {
	public:
		Pipeline(bgfx::ViewId viewId);

		inline bgfx::ViewId viewId() const { return _viewId; }


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


		void beginDraw(const math::float4x4 & viewMatrix, const math::float4x4 & projMatrix);
		inline void beginDraw(MatrixState * matrixState) { beginDraw(matrixState->view().matrix(), matrixState->projection().matrix()); }
		void draw(PolyList * plist, Material * material, const math::float4x4 & modelMatrix, const math::float4x4 & inverseModelMatrix);

        inline void setDepthTest(DepthTest dt) { _depthTest = dt; }
        inline DepthTest depthTest() const { return static_cast<DepthTest>(_depthTest); }
        inline void setBlendFunction(BlendFunction src, BlendFunction dst) { _blendFunctionSrc = src; _blendFunctionDst = dst; }
        inline BlendFunction blendFunctionSrc() const { return static_cast<BlendFunction>(_blendFunctionSrc); }
        inline BlendFunction blendFunctionDst() const { return static_cast<BlendFunction>(_blendFunctionDst); }
        inline void setBlendEquation(BlendEquation be) { _blendEquation = be; }
        inline BlendEquation blendEquation() const { return static_cast<BlendEquation>(_blendEquation); }
        
	protected:
		virtual ~Pipeline();

		bgfx::ViewId _viewId;

		math::color _clearColor = math::color(0.0f, 0.0f, 0.0f, 1.0f);
		float _clearDepth = 1.0f;
		uint16_t _clearFlags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;

		ptr<Shader> _shader;
        
        uint64_t _depthTest = kDepthTestLess;
        uint64_t _blendFunctionSrc = kBlendFunctionDisabled;
        uint64_t _blendFunctionDst = kBlendFunctionDisabled;
        uint64_t _blendEquation = kBlendEquationDisabled;
	};
}
}

#endif
