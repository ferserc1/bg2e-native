
#include <bg2e/base/pipeline.hpp>

#include <stdexcept>

namespace bg2e {
namespace base {


	Pipeline::Pipeline(bgfx::ViewId viewId)
		:_viewId(viewId)
	{

	}

	Pipeline::~Pipeline() {

	}

	void Pipeline::beginDraw() {
		if (!_shader.valid()) {
			throw std::runtime_error("Error drawing polyList: no shader configured in pipeline.");
		}

		bgfx::setViewTransform(_viewId, _view.raw(), _projection.raw());

		if (_clearFlags != 0) {
			bgfx::setViewClear(_viewId, _clearFlags, _clearColor.hexColor(), _clearDepth);
		}

		// TODO: set state
		uint64_t state = 0
			| BGFX_STATE_WRITE_R
			| BGFX_STATE_WRITE_G
			| BGFX_STATE_WRITE_B
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_MSAA
			| UINT64_C(0)// triangle list     BGFX_STATE_PT_TRISTRIP
			;
		bgfx::setState(state);

		_shader->bindFrameUniforms(this);
	}

	void Pipeline::draw(PolyList* plist, Material* material, const math::float4x4& modelMatrix) {
		if (!_shader.valid()) {
			throw std::runtime_error("Error drawing polyList: no shader configured in pipeline.");
		}
		if (!plist || !material) {
			throw std::invalid_argument("Drawing error: invalid material or polyList");
		}


		bgfx::setTransform(modelMatrix.raw());

		_shader->bindUniforms(this, plist, material, modelMatrix);

		bgfx::setVertexBuffer(_viewId, plist->vertexBuffer());
		bgfx::setIndexBuffer(plist->indexBuffer());

		bgfx::submit(_viewId, _shader->programHandle());
	}
}
}
