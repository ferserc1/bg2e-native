#ifndef _bg2e_utils_texture_generator_hpp_
#define _bg2e_utils_texture_generator_hpp_

#include <bg2e/base/texture.hpp>
#include <bg2e/math/vector.hpp>

namespace bg2e {
namespace utils {

	extern base::Texture * textureWithColor(const math::color & c, const math::uint2 & size = math::uint2(4, 4));
	inline base::Texture* blackTexture(const math::uint2& size = math::uint2(4, 4)) { return textureWithColor(math::color(0.0f, 0.0f, 0.0f, 1.0f), size); }
	inline base::Texture* whiteTexture(const math::uint2& size = math::uint2(4, 4)) { return textureWithColor(math::color(1.0f, 1.0f, 1.0f, 1.0f), size); }
	inline base::Texture* normalMapTexture(const math::uint2& size = math::uint2(4, 4)) { return textureWithColor(math::color(0.5f, 0.5f, 1.0f, 1.0f), size); }

}
}


#endif
