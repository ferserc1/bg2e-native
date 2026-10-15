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

#include <bg2e/utils/TextureCache.hpp>
#include <bg2e/db/image.hpp>
#include <bit>

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

std::shared_ptr<render::Texture> TextureCache::load(render::Engine* engine, const std::filesystem::path& filePath)
{
    return load(engine, filePath.string());
}

std::shared_ptr<render::Texture> TextureCache::load(render::Engine* engine, const std::filesystem::path& filePath, const std::string& fileName)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    return load(engine, fullPath);
}

std::shared_ptr<render::Texture> TextureCache::load(render::Engine* engine, const std::string& filePath)
{
    base::Texture defaultSettings;
    defaultSettings.setAddressMode(base::Texture::AddressModeRepeat);
    defaultSettings.setMagFilter(base::Texture::FilterLinear);
    defaultSettings.setMinFilter(base::Texture::FilterLinear);
    defaultSettings.setUseMipmaps(true);
    defaultSettings.setImageFilePath(filePath);
    return load(engine, defaultSettings);
}

std::shared_ptr<render::Texture> TextureCache::load(render::Engine* engine, const base::Texture& settings)
{
    auto filePath = settings.imageFilePath();
    if (filePath == "")
    {
        throw std::runtime_error("TextureCache: could not load texture because the texture data does not contains an image file path");
    }

    // A single image file can be used by views with different sampler settings.
    const auto key = filePath + "|" +
                     std::to_string(settings.magFilter()) + "|" +
                     std::to_string(settings.minFilter()) + "|" +
                     std::to_string(settings.useMipmaps()) + "|" +
                     std::to_string(settings.addressModeU()) + "|" +
                     std::to_string(settings.addressModeV()) + "|" +
                     std::to_string(settings.addressModeW()) + "|" +
                     std::to_string(settings.maxLod()) + "|" +
                     std::to_string(settings.minLod()) + "|" +
                     std::to_string(settings.colorType());
    if (_textures.find(key) == _textures.end())
    {
        std::cout << "Texture not found in cache: " << filePath << std::endl;
        auto texture = std::make_shared<base::Texture>(filePath);
        texture->setMagFilter(settings.magFilter());
        texture->setMinFilter(settings.minFilter());
        texture->setMaxLod(settings.maxLod());
        texture->setMinLod(settings.minLod());
        texture->setMinLod(settings.minLod());
        texture->setImageFilePath(filePath);
        texture->setMagFilter(settings.magFilter());
        texture->setMinFilter(settings.minFilter());
        texture->setUseMipmaps(settings.useMipmaps());
        texture->setAddressMode(settings.addressModeU(), settings.addressModeV(), settings.addressModeW());
        texture->setColorType(settings.colorType());
        auto result = std::make_shared<render::Texture>(engine, texture);
        _textures[key] = result;
    }
    return _textures.find(key)->second;
}

std::shared_ptr<draw::Texture> TextureCache::load(draw::Engine* engine, const std::filesystem::path& filePath)
{
    return load(engine, filePath.string());
}
std::shared_ptr<draw::Texture> TextureCache::load(draw::Engine* engine, const std::filesystem::path& filePath, const std::string& fileName)
{
    return load(engine, filePath / fileName);
}
std::shared_ptr<draw::Texture> TextureCache::load(draw::Engine* engine, const std::string& filePath)
{
    base::Texture settings(filePath);
    settings.setMinFilter(base::Texture::FilterLinear);
    return load(engine, settings);
}
std::shared_ptr<draw::Texture> TextureCache::load(draw::Engine* engine, const base::Texture& settings)
{
    if (!engine || settings.type() != base::Texture::TypeFilesystem || settings.imageFilePath().empty())
    {
        throw std::invalid_argument("Draw texture cache requires an engine and a filesystem source");
    }
    const auto makeKey = [&](const base::Texture& value)
    {
        return std::to_string(reinterpret_cast<uintptr_t>(engine->device())) + "|" +
            std::filesystem::absolute(value.imageFilePath()).lexically_normal().string() + "|" +
            std::to_string(value.minFilter()) + "|" + std::to_string(value.magFilter()) + "|" +
            std::to_string(value.useMipmaps()) + "|" + std::to_string(std::bit_cast<uint32_t>(value.minLod())) + "|" +
            std::to_string(std::bit_cast<uint32_t>(value.maxLod())) + "|" + std::to_string(std::bit_cast<uint32_t>(value.mipLodBias())) + "|" +
            std::to_string(value.addressModeU()) + "|" + std::to_string(value.addressModeV()) + "|" +
            std::to_string(value.addressModeW()) + "|" + std::to_string(value.colorType());
    };
    const auto key = makeKey(settings);
    auto cached = _drawTextures[key].lock();
    if (cached && cached->engine() == engine && cached->image() && cached->image()->isValid() &&
        cached->image()->device() == engine->device() && makeKey(*cached->base()) == key)
    {
        return cached;
    }
    auto result = std::make_shared<draw::Texture>(engine, std::make_shared<base::Texture>(settings));
    _drawTextures[key] = result;
    return result;
}

void TextureCache::emptyCache()
{
    _textures.clear();
    _drawTextures.clear();
}

TextureCache* TextureCache::g_singleton = nullptr;

}
