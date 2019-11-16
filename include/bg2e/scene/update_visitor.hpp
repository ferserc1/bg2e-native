#ifndef _bg2e_scene_render_visitor_hpp_
#define _bg2e_scene_render_visitor_hpp_

#include <bg2e/scene/node_visitor.hpp>
#include <bg2e/scene/node.hpp>
#include <bg2e/base/matrix_state.hpp>

namespace bg2e {
namespace scene {

	class UpdateVisitor : public NodeVisitor {
	public:

		inline void update(Node * sceneRoot, base::MatrixState * matrixState, float delta) {
			_matrixState  = matrixState;
			_delta = delta;
			sceneRoot->accept(this);
		}

		virtual void visit(Node * node) {
			node->update(_matrixState, _delta);
		}

		inline void setDelta(float d) { _delta = d; }
		inline float delta() const { return _delta; }

	protected:
		virtual ~UpdateVisitor() {}

		base::MatrixState * _matrixState = nullptr;
		float _delta = 0.0f;
	};
}
}

#endif
