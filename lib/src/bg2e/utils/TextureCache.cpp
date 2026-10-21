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
    _drawTextures.clear();
}

TextureCache* TextureCache::g_singleton = nullptr;

}
