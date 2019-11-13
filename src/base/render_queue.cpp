
#include <bg2e/base/render_queue.hpp>

namespace bg2e {
namespace base {

	RenderQueue::RenderQueue()
	{

	}

	RenderQueue::~RenderQueue() {

	}

	void RenderQueue::beginDraw(const Camera & cam) {
		_cameraPosition = cam.view().position() * -1.0f;
	}

	void RenderQueue::draw(base::PolyList * plist, base::Material * mat, const math::float4x4 & trx) {
		math::float3 pos = trx.position();
		float distance = (_cameraPosition - pos).length();
		if (mat->isTransparent()) {
			_transparentQueue.push_back({
				plist,
				mat,
				trx,
				true,
				distance
			});
		}
		else {
			_opaqueQueue.push_back({
				plist,
				mat,
				trx,
				true,
				distance
			});
		}
	}

	void RenderQueue::endDraw() {
		// TODO: short transparent queue elements by camera distance
	}
}
}
