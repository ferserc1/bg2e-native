/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/db/image.hpp>

#include <limits>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifdef _WIN32
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

namespace bg2e::db {

namespace {

bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    const auto lowerAscii = [](unsigned char value) {
        return value >= 'A' && value <= 'Z'
            ? static_cast<unsigned char>(value + ('a' - 'A'))
            : value;
    };

    for (std::size_t i = 0; i < left.size(); ++i)
    {
        const auto leftChar = static_cast<unsigned char>(left[i]);
        const auto rightChar = static_cast<unsigned char>(right[i]);
        if (lowerAscii(leftChar) != lowerAscii(rightChar))
        {
            return false;
        }
    }

    return true;
}

}

std::optional<ImageFormat> imageFormatFromExtension(std::string_view extension)
{
    if (!extension.empty() && extension.front() == '.')
    {
        extension.remove_prefix(1);
    }

    if (equalsIgnoreCase(extension, "png"))
    {
        return ImageFormat::PNG;
    }
    if (equalsIgnoreCase(extension, "jpg") || equalsIgnoreCase(extension, "jpeg"))
    {
        return ImageFormat::JPEG;
    }
    if (equalsIgnoreCase(extension, "bmp"))
    {
        return ImageFormat::BMP;
    }
    if (equalsIgnoreCase(extension, "tga"))
    {
        return ImageFormat::TGA;
    }

    return std::nullopt;
}

std::optional<ImageFormat> imageFormatFromPath(const std::filesystem::path& filePath)
{
    return imageFormatFromExtension(filePath.extension().string());
}

std::vector<std::string_view> extensionsForImageFormat(ImageFormat format)
{
    switch (format)
    {
        case ImageFormat::PNG:
            return { ".png" };
        case ImageFormat::JPEG:
            return { ".jpg", ".jpeg" };
        case ImageFormat::BMP:
            return { ".bmp" };
        case ImageFormat::TGA:
            return { ".tga" };
    }

    return {};
}

std::string_view canonicalImageExtension(ImageFormat format)
{
    switch (format)
    {
        case ImageFormat::PNG:
            return ".png";
        case ImageFormat::JPEG:
            return ".jpg";
        case ImageFormat::BMP:
            return ".bmp";
        case ImageFormat::TGA:
            return ".tga";
    }

    return {};
}

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

std::vector<uint8_t> decodeImageRGBA8(
    const uint8_t* encodedData,
    size_t encodedSize,
    uint32_t& width,
    uint32_t& height
) {
    if (!encodedData || encodedSize == 0 || encodedSize > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        throw std::runtime_error("Invalid encoded image data");
    }

    int decodedWidth = 0;
    int decodedHeight = 0;
    int channels = 0;
    auto* pixels = stbi_load_from_memory(encodedData, static_cast<int>(encodedSize),
                                          &decodedWidth, &decodedHeight, &channels, 4);
    if (!pixels || decodedWidth <= 0 || decodedHeight <= 0)
    {
        stbi_image_free(pixels);
        throw std::runtime_error("Could not decode image data");
    }

    const size_t w = static_cast<size_t>(decodedWidth);
    const size_t h = static_cast<size_t>(decodedHeight);
    if (w > std::numeric_limits<size_t>::max() / 4 / h)
    {
        stbi_image_free(pixels);
        throw std::runtime_error("Decoded image dimensions overflow");
    }

    std::vector<uint8_t> result;
    try
    {
        result.assign(pixels, pixels + w * h * 4);
    }
    catch (...)
    {
        stbi_image_free(pixels);
        throw;
    }
    stbi_image_free(pixels);
    width = static_cast<uint32_t>(decodedWidth);
    height = static_cast<uint32_t>(decodedHeight);
    return result;
}

void saveImage(
    const std::filesystem::path& filePath,
    const uint8_t* data,
    uint32_t width,
    uint32_t height,
    uint32_t bpp
) {
    int writtenBytes = 0;
    const auto format = imageFormatFromPath(filePath);
    if (!format)
    {
        throw std::runtime_error("Unsupported image format");
    }

    if (*format == ImageFormat::PNG)
    {
        writtenBytes = stbi_write_png(
            filePath.string().c_str(),
            width, height, bpp,
            data,
            0
        );
    }
    else if (*format == ImageFormat::JPEG)
    {
        static const int quality = 100;
        writtenBytes = stbi_write_jpg(
            filePath.string().c_str(),
            width, height, bpp,
            data,
            quality
        );
    }
    else if (*format == ImageFormat::BMP)
    {
        writtenBytes = stbi_write_bmp(
            filePath.string().c_str(),
            width, height, bpp,
            data
        );
    }
    else if (*format == ImageFormat::TGA)
    {
        writtenBytes = stbi_write_tga(
            filePath.string().c_str(),
            width, height, bpp,
            data
        );
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
