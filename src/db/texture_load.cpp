
#include <bg2e/db/texture_load.hpp>
#include <bg2e/db/image_load.hpp>

namespace bg2e {
namespace db {

	bg2e::base::Texture* loadTexture(const bg2e::base::path& filePath) {
		ptr<base::Image> image = loadImage(filePath);
		ptr<base::Texture> texture = new base::Texture();
		texture->create(image);
		return texture.release();
	}

	bg2e::base::Texture* loadTexture(const std::string& filePath) {
		return loadTexture(base::path(filePath));
	}

	bg2e::base::Texture* loadTexture(const char* filePath) {
		return loadTexture(base::path(filePath));
	}

}
}
