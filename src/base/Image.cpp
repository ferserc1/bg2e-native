
#include <bg2e/base/Image.hpp>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

namespace bg2e {
namespace base {

std::unordered_map<std::string, std::shared_ptr<Image>> Image::g_imageCache;

void Image::ClearCache()
{
    g_imageCache.clear();
}

void Image::ClearCacheFile(const std::string& file)
{
    auto it = g_imageCache.find(file);
    if (it != g_imageCache.end())
    {
        g_imageCache.erase(it);
    }
}

std::shared_ptr<Image> Image::LoadFromFile(const std::string& fileName)
{
    if (g_imageCache.find(fileName) != g_imageCache.end())
    {
        std::cout << "Image found in cache: '" << fileName << "'. Skip load" << std::endl;
        return g_imageCache[fileName];
    }
    else
    {
        std::cout << "Image not found in cache: '" << fileName << "'. Loading from file" << std::endl;
        int w, h, c;
        stbi_uc* data = stbi_load(fileName.c_str(), &w, &h, &c, STBI_rgb_alpha);
        
        if (!data)
        {
            throw std::runtime_error("Could not load image from file '" + fileName + "'");
        }
        
        std::shared_ptr<Image> result = std::make_shared<Image>();
        result->_imageData = static_cast<Byte*>(data);
        result->_size = Size{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
        if (c == 3)
        {
            result->_format = ImageComponentFormat::RGB;
        }
        else if (c == 4)
        {
            result->_format = ImageComponentFormat::RGBA;
        }
        result->_imageSource = ImageSource::LoadFromFile;
        
        g_imageCache[fileName] = result;
        return result;
    }
}

Image::Image()
{
    
}

Image::~Image()
{
    destroy();
}

void Image::destroy()
{
    std::cout << "Destroy image" << std::endl;
    if (_imageSource == ImageSource::Setter)
    {
        delete [] _imageData;
    }
    else if (_imageSource == ImageSource::LoadFromFile)
    {
        stbi_image_free(_imageData);
    }
    _imageData = nullptr;
    _size = Size{ 0, 0 };
    _format = ImageComponentFormat::RGB;
}


}
}

