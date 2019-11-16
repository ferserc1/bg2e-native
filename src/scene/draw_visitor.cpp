
#include <bg2e/scene/draw_visitor.hpp>

#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {


	DrawVisitor::~DrawVisitor() {

	}

	void DrawVisitor::visit(Node * node) {
		node->draw(*_renderQueue, _matrixState);
	}

}
}
