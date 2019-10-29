
#include <bg2e/db/image_load.hpp>
#include <bg2e/math/vector.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ios>

namespace bg2e {
namespace db {

	bg2e::base::image* loadImage(const bg2e::base::path& filePath) {
		int w, h, bpp;
		unsigned char* data = stbi_load(filePath.toString().c_str(), &w, &h, &bpp, 4);
		if (!data) {
			throw std::ios_base::failure("Error loading image at path: " + filePath.toString());
		}
		auto result = new bg2e::base::image(
			data,
			bg2e::math::uint2(static_cast<uint32_t>(w), static_cast<uint32_t>(h)),
			4, bg2e::base::image::kFormatRGBA
		);
		result->setFilePath(filePath);
		return result;
	}

	bg2e::base::image* loadImage(const std::string& filePath) {
		return loadImage(bg2e::base::path(filePath));
	}

	bg2e::base::image* loadImage(const char* filePath) {
		return loadImage(bg2e::base::path(filePath));
	}
}
}
