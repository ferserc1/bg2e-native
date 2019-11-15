#ifndef _bg2e_base_render_queue_hpp_
#define _bg2e_base_render_queue_hpp_

#include <bg2e/base/drawable_element.hpp>
#include <bg2e/base/camera.hpp>
#include <bg2e/base/pipeline.hpp>

#include <vector>

namespace bg2e {
namespace base {

	class RenderQueue {
	public:
		RenderQueue();
		virtual ~RenderQueue();

		void begin(const Camera & cam);
		void addPolyList(base::PolyList * plist, base::Material * mat, const math::float4x4 & trx);
		void end();

		inline const base::DrawableElementVector & opaqueQueue() const { return _opaqueQueue; }
        inline base::DrawableElementVector & opaqueQueue() { return _opaqueQueue; }
		inline const base::DrawableElementVector & transparentQueue() const { return _transparentQueue; }
		inline base::DrawableElementVector & transparentQueue() { return _transparentQueue; }
        
	protected:

		math::float3 _cameraPosition;
		base::DrawableElementVector _opaqueQueue;
		base::DrawableElementVector _transparentQueue;
	};

}
}

#endif
