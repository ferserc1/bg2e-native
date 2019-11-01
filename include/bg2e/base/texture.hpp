#ifndef _bg2e_base_texture_hpp_
#define _bg2e_base_texture_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/image.hpp>

#include <bgfx/bgfx.h>

namespace bg2e {
namespace base{

	class Texture : public ReferencedPointer {
	public:
		Texture();

		void create(Image * image);
		inline void create(ptr<Image>& image) { create(image.getPtr()); }

		inline const Image * image() const { return _image.getPtr(); }

		inline const bgfx::TextureHandle& textureHandle() const { return _texture; }
	
	protected:
		virtual ~Texture();

		bgfx::TextureHandle _texture;

		ptr<Image> _image;
	};

}
}
#endif
