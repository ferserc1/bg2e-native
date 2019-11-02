
#include <bg2e/utils/texture_generator.hpp>

#include <bg2e/base/image.hpp>

namespace bg2e {
namespace utils {

	base::Texture* textureWithColor(const math::color& c, const math::uint2& size) {
		ptr<base::Texture> result = new base::Texture();
		ptr<base::Image> image = new base::Image();

		uint8_t bpp = 4;
		uint32_t dataSize = size.width() * size.height() * bpp;
		uint8_t * data = new uint8_t[dataSize];
		
		for (uint32_t y = 0; y < size.height(); ++y) {
			for (uint32_t x = 0; x < size.width() * bpp; x += bpp) {
				uint32_t i = y * size.width() * bpp + x;
				data[i    ] = static_cast<uint8_t>(c.r() * 255.0f);
				data[i + 1] = static_cast<uint8_t>(c.g() * 255.0f);
				data[i + 2] = static_cast<uint8_t>(c.b() * 255.0f);
				data[i + 3] = static_cast<uint8_t>(c.a() * 255.0f);
			}
		}
		image->setData(data, size, bpp,base::Image::kFormatRGBA);
		result->create(image);

		return result.release();
	}

}
}
