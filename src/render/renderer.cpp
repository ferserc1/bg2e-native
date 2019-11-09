
#include <bg2e/render/renderer.hpp>

#include <bg2e/render/forward_render_layer.hpp>

namespace bg2e {
namespace render {

    Renderer * Renderer::Create(bgfx::ViewId targetView) {
        return new Renderer(
			targetView, 
			new ForwardRenderLayer(targetView, true), 
			new ForwardRenderLayer(targetView, true));
    }

    Renderer::Renderer(bgfx::ViewId viewId, RenderLayer * opaqueLayer, RenderLayer * transparentLayer)
		:_viewId(viewId)
		,_opaqueLayer(opaqueLayer)
		,_transparentLayer(transparentLayer)
    {

    }

    Renderer::~Renderer() {
		delete _transparentLayer;
		delete _opaqueLayer;
    }

	void Renderer::resize(scene::Node * sceneRoot, uint32_t w, uint32_t h) {
		
	}

	void Renderer::update(scene::Node * sceneRoot, float delta) {

	}

	void Renderer::draw(scene::Node * sceneRoot) {

	}

}
}