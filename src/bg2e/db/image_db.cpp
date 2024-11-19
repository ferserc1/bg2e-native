
#include <bg2e/db/image.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

namespace bg2e::db {

bg2e::base::Image * loadImage(const std::filesystem::path& filePath)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.string().c_str(), &width, &height, &channels, 4);
    if (!data)
    {
        throw std::runtime_error("Error loading image at path " + filePath.string());
    }
    
    return new bg2e::base::Image(
        data,
        uint32_t(width),
        uint32_t(height),
        uint32_t(channels)
    );
}

}
