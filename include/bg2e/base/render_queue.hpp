#ifndef _bg2e_base_render_queue_hpp_
#define _bg2e_base_render_queue_hpp_

#include <bg2e/base/drawable_element.hpp>
#include <bg2e/base/camera.hpp>

#include <vector>

namespace bg2e {
namespace base {

	class RenderQueue {
	public:
		RenderQueue();
		virtual ~RenderQueue();

		void beginDraw(const Camera & cam);
		void draw(base::PolyList * plist, base::Material * mat, const math::float4x4 & trx);
		void endDraw();
		
	protected:

		math::float3 _cameraPosition;
		base::DrawableElementVector _opaqueQueue;
		base::DrawableElementVector _transparentQueue;
	};

}
}

#endif
