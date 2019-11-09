#ifndef _bg2e_render_render_layer_hpp_
#define _bg2e_render_render_layer_hpp_

#include <bgfx/bgfx.h>

#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace render {

	class RenderLayer {
	public:
		RenderLayer(bgfx::ViewId viewId, bool transparent) :_viewId(viewId), _transparent(transparent) {}
		virtual ~RenderLayer() {}

		virtual void resize(scene::Node * node, uint32_t w, uint32_t h) = 0;
		virtual void update(scene::Node * node, float delta) = 0;
		virtual void draw(scene::Node * node) = 0;

		inline bgfx::ViewId viewId() const { return _viewId; }
		inline bool isTransparent() const { return _transparent; }

	protected:
		bgfx::ViewId _viewId;
		bool _transparent;
	};

}
}

#endif
