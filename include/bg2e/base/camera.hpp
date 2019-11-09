#ifndef _bg2e_base_camera_hpp_
#define _bg2e_base_camera_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/projection_strategy.hpp>

namespace bg2e {
namespace base {

	class Camera : public ReferencedPointer {
	public:
		Camera();

	protected:
		virtual ~Camera();
	};

}
}

#endif
