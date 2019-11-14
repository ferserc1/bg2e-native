#ifndef _bg2e_scene_render_visitor_hpp_
#define _bg2e_scene_render_visitor_hpp_

#include <bg2e/base/pipeline.hpp>
#include <bg2e/scene/node_visitor.hpp>
#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {

	class UpdateVisitor : public NodeVisitor {
	public:

		inline void update(Node * sceneRoot, base::Pipeline * pipeline, float delta) {
			_pipeline = pipeline;
			_delta = delta;
			sceneRoot->accept(this);
		}

		virtual void visit(Node * node) {
			node->update(_pipeline, _delta);
		}

		inline void setDelta(float d) { _delta = d; }
		inline float delta() const { return _delta; }

	protected:
		virtual ~UpdateVisitor() {}

		base::Pipeline * _pipeline = nullptr;
		float _delta = 0.0f;
	};
}
}

#endif
