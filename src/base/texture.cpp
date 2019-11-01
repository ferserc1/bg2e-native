
#include <bg2e/base/texture.hpp>

#include <stdexcept>


namespace bg2e {
namespace base {


	Texture::Texture()
	{

	}

	Texture::~Texture() {
		bgfx::destroy(_texture);
	}

	void Texture::create(Image* image) {
		if (!image->valid()) {
			throw std::invalid_argument("Error creating texture image");
		}
		_image = image;
		_texture = bgfx::createTexture2D(
			static_cast<uint16_t>(image->size().width()),
			static_cast<uint16_t>(image->size().height()),
			false,
			1,
			bgfx::TextureFormat::RGBA8,
			0,
			image->memoryRef()
		);

		if (bgfx::isValid(_texture)) {
			bgfx::setName(_texture, image->filePath().toString().c_str());
		}
		else {
			throw std::runtime_error("Error creating texture from image. The texture is invalid");
		}
	}

}
}