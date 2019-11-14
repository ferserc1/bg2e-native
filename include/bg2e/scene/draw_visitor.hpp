#ifndef _bg2e_scene_draw_vistor_hpp_
#define _bg2e_scene_draw_vistor_hpp_

#include <bg2e/scene/node_visitor.hpp>

#include <bg2e/base/pipeline.hpp>
#include <bg2e/base/render_queue.hpp>

#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {

	class DrawVisitor : public NodeVisitor {
	public:

		inline void draw(Node * sceneRoot, base::RenderQueue * renderQueue, base::Pipeline * pipeline) {
			_renderQueue = renderQueue;
			_pipeline = pipeline;
			sceneRoot->accept(this);
		}

		virtual void visit(Node * node);

	protected:
		virtual ~DrawVisitor();

		base::RenderQueue * _renderQueue;
		base::Pipeline * _pipeline;
	};
}
}

#endif
