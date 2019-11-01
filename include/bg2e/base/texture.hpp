#ifndef _bg2e_base_texture_hpp_
#define _bg2e_base_texture_hpp_

#include <bg2e/base/referenced_pointer.hpp>

#include <bgfx/bgfx.h>

namespace bg2e {
namespace base{

	class Texture : public ReferencedPointer {
	public:
		Texture();


	protected:
		virtual ~Texture();
	};

}
}
#endif
