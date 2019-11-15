
#ifndef _bg2e_base_matrix_state_hpp_
#define _bg2e_base_matrix_state_hpp_

#include <bg2e/base/matrix_stack.hpp>
#include <bg2e/base/referenced_pointer.hpp>

namespace bg2e {
namespace base {

    class MatrixState : public ReferencedPointer {
    public:
        MatrixState();
        
        inline MatrixStack & projection() { return _projectionMatrixStack; }
        inline const MatrixStack & projection() const { return _projectionMatrixStack; }
        inline MatrixStack & view() { return _viewMatrixStack; }
        inline const MatrixStack & view() const { return _viewMatrixStack; }
        inline MatrixStack & model() { return _modelMatrixStack; }
        inline const MatrixStack & model() const { return _modelMatrixStack; }
        
    protected:
        virtual ~MatrixState();
        
        MatrixStack _projectionMatrixStack;
        MatrixStack _viewMatrixStack;
        MatrixStack _modelMatrixStack;
    };

}
}

#endif

