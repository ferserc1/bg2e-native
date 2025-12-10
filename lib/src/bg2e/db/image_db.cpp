
#include <bg2e/db/image.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(push)
#pragma warning(disable:4996)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#pragma warning(pop)
#pragma GCC diagnostic pop

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

void saveImage(
    const std::filesystem::path& filePath,
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint32_t bpp
) {
    auto ext = filePath.extension();
    int writtenBytes = 0;
    if (ext == ".png")
    {
        writtenBytes = stbi_write_png(
            filePath.string().c_str(),
            width, height, bpp,
            data,
            0
        );
    }
    else if (ext == ".jpg" || ext == ".jpeg")
    {
        static const int quality = 100;
        writtenBytes = stbi_write_jpg(
            filePath.string().c_str(),
            width, height, bpp,
            data,
            quality
        );
    }
    else if (ext == ".bmp")
    {
        writtenBytes = stbi_write_bmp(
            filePath.string().c_str(),
            width, height, bpp,
            data
        );
    }
    else if (ext == ".tga")
    {
        writtenBytes = stbi_write_tga(
            filePath.string().c_str(),
            width, height, bpp,
            data
        );
    }
    else
    {
        throw std::runtime_error("Unsupported image format");
    }

    if (writtenBytes == 0)
    {
        throw std::runtime_error("Error writing image at path " + filePath.string());
    }
}


void saveImage(
    const std::filesystem::path& basePath,
    const std::string& fileName,
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint32_t bpp
) {
    auto fullPath = basePath / fileName;
    saveImage(fullPath, data, width, height, bpp);
}


}
