
#include <bg2e/base/matrix_state.hpp>


namespace bg2e {
namespace base {

    MatrixState::MatrixState()
    {
        
    }

    MatrixState::~MatrixState() {
        
    }

	void MatrixState::beginFrame() {
		_projectionMatrixStack.beginFrame();
		_viewMatrixStack.beginFrame();
		_modelMatrixStack.beginFrame();
	}
}
}


