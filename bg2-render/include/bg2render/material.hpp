#ifndef _bg2render_material_hpp_
#define _bg2render_material_hpp_

#include <bg2render/texture.hpp>

namespace bg2render{

	class Material {
	public:
		Material() {}

		inline void setTexture(bg2render::Texture* tex) { _texture = std::shared_ptr<bg2render::Texture>(tex); }
		inline const bg2render::Texture* texture() const { return _texture.get(); }

	protected:
		std::shared_ptr<bg2render::Texture> _texture;
	};
}

#endif
