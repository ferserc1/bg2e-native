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

#include <bg2e/draw/ImageLoadContext.hpp>
#include <bg2e/db/image.hpp>

#include <stdexcept>

namespace bg2e::draw {

ImageLoadContext::ImageLoadContext(Engine* engine) : _engine(engine)
{
    if (!engine)
    {
        throw std::invalid_argument("ImageLoadContext requires an engine");
    }
    engine->device();
}


Engine* ImageLoadContext::engine() const
{
    return _engine;
}


std::string ImageLoadContext::sourceKey(const std::filesystem::path& path)
{
    if (path.empty())
    {
        throw std::invalid_argument("ImageLoadContext requires a filesystem source");
    }
    return std::filesystem::absolute(path).lexically_normal().string();
}


void ImageLoadContext::registerPixels(const std::filesystem::path& path,
    std::shared_ptr<const base::Image> pixels)
{
    if (!pixels || (!pixels->data() && !pixels->dataf()) ||
        pixels->width() == 0 || pixels->height() == 0 || pixels->channels() != 4)
    {
        throw std::invalid_argument("ImageLoadContext requires decoded RGBA pixels");
    }
    const auto key = sourceKey(path);
    if (!_pixels.emplace(key, std::move(pixels)).second)
    {
        throw std::invalid_argument("ImageLoadContext source is already registered or decoded");
    }
}


std::shared_ptr<Texture> ImageLoadContext::load(const base::Texture& settings)
{
    if (settings.type() != base::Texture::TypeFilesystem)
    {
        // Procedural sources deliberately bypass image reuse.
        return std::make_shared<Texture>(_engine, std::make_shared<base::Texture>(settings));
    }
    const auto source = sourceKey(settings.imageFilePath());
    // Sampler settings are independent. Mip generation and color interpretation
    // remain part of image identity; the latter allows future color-aware mips.
    const auto key = source + "|" + std::to_string(settings.useMipmaps()) +
        "|" + std::to_string(settings.colorType());
    auto result = std::make_shared<Texture>(_engine);
    if (auto found = _images.find(key); found != _images.end())
    {
        result->load(std::make_shared<base::Texture>(settings), found->second);
        return result;
    }
    auto found = _pixels.find(source);
    if (found == _pixels.end())
    {
        auto pixels = std::shared_ptr<const base::Image>(db::loadImage(source));
        found = _pixels.emplace(source, std::move(pixels)).first;
    }
    result->load(*found->second, settings);
    _images.emplace(key, result->imageResource());
    return result;
}


std::shared_ptr<Texture> ImageLoadContext::neutralTexture(bool normal)
{
    auto& image = _neutralImages[normal ? 1 : 0];
    const auto color = normal ? base::Color(0.5f, 0.5f, 1.0f, 1.0f) : base::Color::White();
    if (!image)
    {
        auto result = Texture::colorTexture(_engine, color);
        image = result->imageResource();
        return result;
    }
    auto result = std::make_shared<Texture>(_engine);
    auto settings = std::make_shared<base::Texture>();
    settings->setUseMipmaps(false);
    settings->setColorType(base::Color::TypeLinear);
    result->load(std::move(settings), image);
    // Keep the existing empty-path reload contract without another GPU upload.
    auto data = std::make_unique<uint8_t[]>(4);
    data[0] = data[1] = uint8_t(normal ? 128 : 255);
    data[2] = data[3] = 255;
    result->_memorySource = std::make_unique<base::Image>(data.get(), 1, 1, 4);
    data.release();
    return result;
}

}
