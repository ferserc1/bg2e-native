
#include <bg2-db/image_load.hpp>
#include <bg2-math/vector.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <bg2-db/stb_image.h>

#include <ios>

namespace bg2db {

    bg2base::image * loadImage(const bg2base::path & filePath) {
        int w, h, bpp;
        unsigned char * data = stbi_load(filePath.toString().c_str(), &w, &h, &bpp, 4);
        if (!data) {
            throw std::ios_base::failure("Error loading image at path: " + filePath.toString());
        }
        return new bg2base::image(
            data,
            bg2math::uint2(static_cast<uint32_t>(w),static_cast<uint32_t>(h)),
            4,bg2base::image::kFormatRGBA
        );
    }
    
    bg2base::image * loadImage(const std::string & filePath) {
        return loadImage(bg2base::path(filePath));
    }
    
    bg2base::image * loadImage(const char * filePath) {
        return loadImage(bg2base::path(filePath));
    }
}

