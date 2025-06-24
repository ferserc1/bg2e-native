
#include <bg2e/db/image.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

namespace bg2e::db {

bg2e::base::Image * loadImage(const std::filesystem::path& filePath)
{
    if (filePath.extension() == ".hdr")
    {
        int width, height, channels;
        float* data = stbi_loadf(filePath.string().c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            throw std::runtime_error("Error loading image at path " + filePath.string());
        }
        
        auto result = new bg2e::base::Image(
            data,
            uint32_t(width),
            uint32_t(height),
            4
        );
        
        result->setPath(filePath.string());
        return result;
    }
    else{
        int width, height, channels;
        unsigned char* data = stbi_load(filePath.string().c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            throw std::runtime_error("Error loading image at path " + filePath.string());
        }
        
        auto result = new bg2e::base::Image(
            data,
            uint32_t(width),
            uint32_t(height),
            4
        );
        
        result->setPath(filePath.string());
        return result;
    }
}

bg2e::base::Image * loadImage(const std::filesystem::path& basePath, const std::string& fileName)
{
    auto fullPath = basePath;
    fullPath.append(fileName);
    
    return loadImage(fullPath);
}

}
