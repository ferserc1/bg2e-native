
#include <bg2e/utils/texture_cache.hpp>

#include <bg2e/utils/texture_generator.hpp>

namespace bg2e {
namespace utils {

	static const char * kBlackTextureId = "texture_cache_black";
	static const char * kWhiteTextureId = "texture_cache_white";
	static const char * kNormalTextureId = "texture_cache_normal";

	TextureCache* TextureCache::s_singleton = nullptr;

	TextureCache* TextureCache::Get() {
		if (s_singleton == nullptr) {
			s_singleton = new TextureCache();
		}
		return s_singleton;
	}

	void TextureCache::Destroy() {
		delete s_singleton;
	}

	TextureCache::TextureCache() {
		add(kBlackTextureId, utils::blackTexture());
		add(kWhiteTextureId, utils::whiteTexture());
		add(kNormalTextureId, utils::normalMapTexture());
	}

	void TextureCache::add(const std::string & id, base::Texture * tex) {
		_registry[id] = tex;
	}

	base::Texture * TextureCache::find(const std::string & id) {
		if (_registry.find(id) == _registry.end()) {
			return nullptr;
		}
		else {
			return _registry[id].getPtr();
		}
	}

	base::Texture * TextureCache::blackTexture() {
		return _registry[kBlackTextureId].getPtr();
	}

	base::Texture * TextureCache::whiteTexture() {
		return _registry[kWhiteTextureId].getPtr();
	}

	base::Texture * TextureCache::normalMapTexture() {
		return _registry[kNormalTextureId].getPtr();
	}

}
}
