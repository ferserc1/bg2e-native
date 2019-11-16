
#include <bg2e/base/render_queue.hpp>

namespace bg2e {
namespace base {

	RenderQueue::RenderQueue()
	{

	}

	RenderQueue::~RenderQueue() {

	}

	void RenderQueue::begin(const Camera & cam) {
		_opaqueQueue.clear();
		_transparentQueue.clear();
		_cameraPosition = cam.view().position() * -1.0f;
	}

	void RenderQueue::addPolyList(base::PolyList * plist, base::Material * mat, const math::float4x4 & trx, const math::float4x4 & invTrx) {
		math::float3 pos = trx.position();
		float distance = (_cameraPosition - pos).length();
		if (mat->isTransparent()) {
			_transparentQueue.push_back({
				plist,
				mat,
				trx,
				true,
				distance,
				invTrx,
				true
			});
		}
		else {
			_opaqueQueue.push_back({
				plist,
				mat,
				trx,
				true,
				distance,
				invTrx,
				true
			});
		}
	}

	void RenderQueue::end() {
		// TODO: short transparent queue elements by camera distance
	}

}
}
