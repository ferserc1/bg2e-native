#ifndef _bg2e_utils_texture_cache_hpp_
#define _bg2e_utils_texture_cache_hpp_

#include <bg2e/base/texture.hpp>

#include <unordered_map>

namespace bg2e {
namespace utils {


	class TextureCache {
	public:
		static TextureCache * Get();
		static void Destroy();

		TextureCache();

		void add(const std::string & id, base::Texture * tex);
		base::Texture * find(const std::string & id);

		base::Texture * blackTexture();
		base::Texture * whiteTexture();
		base::Texture * normalMapTexture();

	protected:
		static TextureCache * s_singleton;

		std::unordered_map<std::string, ptr<base::Texture>> _registry;
	};

}
}

#endif
