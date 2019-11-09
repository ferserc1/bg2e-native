#ifndef _bg2e_render_forward_render_layer_hpp_
#define _bg2e_render_forward_render_layer_hpp_

#include <bg2e/render/render_layer.hpp>

namespace bg2e {
namespace render {

	class ForwardRenderLayer : public RenderLayer {
	public:
		ForwardRenderLayer(bgfx::ViewId viewId, bool transparent);
		virtual ~ForwardRenderLayer();

		virtual void resize(scene::Node * node, uint32_t w, uint32_t h);
		virtual void update(scene::Node * node, float delta);
		virtual void draw(scene::Node * node);
	};

}
}

#endif
