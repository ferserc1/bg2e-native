#ifndef _bg2e_render_renderer_hpp_
#define _bg2e_render_renderer_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/render/render_layer.hpp>
#include <bg2e/scene/node.hpp>

#include <bgfx/bgfx.h>

namespace bg2e {
namespace render {


    class Renderer : public base::ReferencedPointer {
    public:
        static Renderer * Create(bgfx::ViewId targetView);

		inline bgfx::ViewId viewId() const { return _viewId; }

		void resize(scene::Node * sceneRoot, uint32_t w, uint32_t h);
		void update(scene::Node* sceneRoot, float delta);
		void draw(scene::Node* sceneRoot);

    protected:
        Renderer(bgfx::ViewId viewId, RenderLayer * opaqueLayer, RenderLayer * transparentLayer);
        virtual ~Renderer();

		bgfx::ViewId _viewId;
		RenderLayer * _opaqueLayer;
		RenderLayer * _transparentLayer;
    };

}
}

#endif
