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

std::shared_ptr<render::Texture> TextureCache::load(render::Vulkan * vk, const std::filesystem::path& filePath)
{
    base::Texture defaultSettings;
    defaultSettings.setAddressMode(base::Texture::AddressModeRepeat);
    defaultSettings.setMagFilter(base::Texture::FilterLinear);
    defaultSettings.setMinFilter(base::Texture::FilterLinear);
    defaultSettings.setUseMipmaps(true);
    return load(vk, filePath, defaultSettings);
}

std::shared_ptr<render::Texture> TextureCache::load(render::Vulkan * vk, const std::filesystem::path& filePath, const std::string& fileName)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    return load(vk, fullPath);
}

std::shared_ptr<render::Texture> TextureCache::load(render::Vulkan * vk, const std::filesystem::path& filePath, const base::Texture& settings)
{
    if (_textures.find(filePath.string()) == _textures.end())
    {
        std::cout << "Texture not found in cache: " << filePath << std::endl;
        auto image = db::loadImage(filePath);
        auto texture = new base::Texture(image);
        texture->setMagFilter(settings.magFilter());
        texture->setMinFilter(settings.minFilter());
        texture->setMaxLod(settings.maxLod());
        texture->setMinLod(settings.minLod());
        texture->setCacheHash(image->path());
        texture->setMagFilter(settings.magFilter());
        texture->setMinFilter(settings.minFilter());
        texture->setUseMipmaps(settings.useMipmaps());
        texture->setAddressMode(settings.addressModeU(), settings.addressModeV(), settings.addressModeW());
        auto result = std::make_shared<render::Texture>(vk, texture);
        _textures[filePath.string()] = result;
    }
    return _textures.find(filePath.string())->second;
}

std::shared_ptr<render::Texture> TextureCache::load(render::Vulkan * vk, const std::filesystem::path& filePath, const std::string & fileName, const base::Texture& settings)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    return load(vk, fullPath, settings);
}

std::shared_ptr<render::Texture> TextureCache::load(render::Vulkan * vk, std::shared_ptr<base::Texture> texture)
{
    auto hash = texture->cacheHash();
    if (hash == "")
    {
        throw std::runtime_error("Could not create cached texture: the supplied texture does not contains a valid hash");
    }
    
    if (_textures.find(hash) == _textures.end())
    {
        std::cout << "Texture not cached. Adding texture to cache: " << hash << std::endl;
        auto result = std::make_shared<render::Texture>(vk, texture.get());
        _textures[hash] = result;
    }
    
    return _textures[hash];
}

void TextureCache::emptyCache()
{
    _textures.clear();
}

TextureCache* TextureCache::g_singleton = nullptr;

}
