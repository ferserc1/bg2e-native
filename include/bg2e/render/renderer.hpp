#ifndef _bg2e_render_renderer_hpp_
#define _bg2e_render_renderer_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/scene/camera.hpp>
#include <bg2e/render/render_layer.hpp>
#include <bg2e/scene/node.hpp>

#include <bgfx/bgfx.h>

namespace bg2e {
namespace render {


    class Renderer : public base::ReferencedPointer {
    public:
        static Renderer * Create(bgfx::ViewId targetView);

		inline bgfx::ViewId viewId() const { return _viewId; }
        
        inline void setMainCamera(scene::Camera * camera) { _mainCamera = camera; }
        inline void setMainCamera(ptr<scene::Camera> camera) { _mainCamera = camera; }
        inline scene::Camera * mainCamera() { return _mainCamera.getPtr(); }
        inline const scene::Camera * mainCamera() const { return _mainCamera.getPtr(); }

		void resize(scene::Node * sceneRoot, uint32_t w, uint32_t h);
		void update(scene::Node * sceneRoot, float delta);
		void draw(scene::Node * sceneRoot);

    protected:
        Renderer(bgfx::ViewId viewId, RenderLayer * opaqueLayer, RenderLayer * transparentLayer);
        virtual ~Renderer();

		bgfx::ViewId _viewId;
		RenderLayer * _opaqueLayer;
		RenderLayer * _transparentLayer;
        
        ptr<scene::Camera> _mainCamera;
    };

}
}

#endif
