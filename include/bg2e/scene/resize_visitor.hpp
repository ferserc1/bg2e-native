#ifndef _bg2e_scene_resize_visitor_hpp_
#define _bg2e_scene_resize_visitor_hpp_

#include <bg2e/scene/node_visitor.hpp>
#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {

	class ResizeVisitor : public NodeVisitor {
	public:

		inline void resize(Node * sceneRoot, uint32_t w, uint32_t h) {
			_width = w;
			_height = h;
			sceneRoot->accept(this);
		}

		virtual void visit(Node * node) {
			node->resize(_width, _height);
		}

		inline void setSize(uint32_t w, uint32_t h) { _width = w; _height = h; }
		inline uint32_t width() const { return _width; }
		inline uint32_t height() const { return _height; }

	protected:
		virtual ~ResizeVisitor() {}

		uint32_t _width;
		uint32_t _height;
	};
}
}

#endif

