//
//  TextureCache.cpp

#include <bg2e/utils/TextureCache.hpp>
#include <bg2e/db/image.hpp>

namespace bg2e::utils {

TextureCache& TextureCache::get()
{
    if (g_singleton == nullptr)
    {
        g_singleton = new TextureCache();
    }
    return *g_singleton;
}

void TextureCache::destroy()
{
    if (g_singleton != nullptr)
    {
        delete g_singleton;
        g_singleton = nullptr;
    }
}

std::shared_ptr<render::Texture> TextureCache::load(render::Engine * engine, const std::filesystem::path& filePath)
{
    return load(engine, filePath.string());
}

std::shared_ptr<render::Texture> TextureCache::load(render::Engine * engine, const std::filesystem::path& filePath, const std::string& fileName)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    return load(engine, fullPath);
}

std::shared_ptr<render::Texture> TextureCache::load(render::Engine * engine, const std::string& filePath)
{
    base::Texture defaultSettings;
    defaultSettings.setAddressMode(base::Texture::AddressModeRepeat);
    defaultSettings.setMagFilter(base::Texture::FilterLinear);
    defaultSettings.setMinFilter(base::Texture::FilterLinear);
    defaultSettings.setUseMipmaps(true);
    defaultSettings.setImageFilePath(filePath);
    return load(engine, defaultSettings);
}

std::shared_ptr<render::Texture> TextureCache::load(render::Engine * engine, const base::Texture& settings)
{
    auto filePath = settings.imageFilePath();
    if (filePath == "")
    {
        throw std::runtime_error("TextureCache: could not load texture because the texture data does not contains an image file path");
    }
    
    if (_textures.find(filePath) == _textures.end())
    {
        std::cout << "Texture not found in cache: " << filePath << std::endl;
        auto texture = std::make_shared<base::Texture>(filePath);
        texture->setMagFilter(settings.magFilter());
        texture->setMinFilter(settings.minFilter());
        texture->setMaxLod(settings.maxLod());
        texture->setMinLod(settings.minLod());
        texture->setImageFilePath(filePath);
        texture->setMagFilter(settings.magFilter());
        texture->setMinFilter(settings.minFilter());
        texture->setUseMipmaps(settings.useMipmaps());
        texture->setAddressMode(settings.addressModeU(), settings.addressModeV(), settings.addressModeW());
        auto result = std::make_shared<render::Texture>(engine, texture);
        _textures[filePath] = result;
    }
    return _textures.find(filePath)->second;
}

void TextureCache::emptyCache()
{
    _textures.clear();
}

TextureCache* TextureCache::g_singleton = nullptr;

}
