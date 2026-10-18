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

#include <bg2e/ui/BakeLightmapPreview.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/Texture.hpp>
#include <bg2e/base/Texture.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <stdexcept>

namespace bg2e::ui {

BakeLightmapPreview::~BakeLightmapPreview()
{
    clear();
}

void BakeLightmapPreview::setResult(UserInterface& ui, draw::Engine& engine, const bake::LightmapResult& result)
{
    if (!result.image || result.extent.width == 0 || result.extent.height == 0 ||
        result.image->size().width != result.extent.width || result.image->size().height != result.extent.height)
    {
        throw std::invalid_argument("BakeLightmapPreview requires a complete image with a matching nonzero extent");
    }
    setImage(ui, engine, result.image);
}

void BakeLightmapPreview::setImage(UserInterface& ui, draw::Engine& engine, std::shared_ptr<gpu::Image> image)
{
    if (!image)
    {
        throw std::invalid_argument("BakeLightmapPreview requires a non-null image");
    }
    if (image->currentLayout() != gpu::ImageLayout::ShaderReadOnly)
    {
        throw std::invalid_argument("BakeLightmapPreview image must be in shader-read-only layout");
    }
    if (engine.device() != image->device())
    {
        throw std::invalid_argument("BakeLightmapPreview image and draw engine must use the same device");
    }

    clear();
    auto settings = std::make_shared<base::Texture>();
    settings->setUseMipmaps(false);
    settings->setColorType(base::Color::TypeLinear);
    settings->setAddressMode(base::Texture::AddressModeClampToEdge);
    const auto filter = engine.device()->formatCapabilities(image->pixelFormat()).linearFiltering
        ? base::Texture::FilterLinear : base::Texture::FilterNearest;
    settings->setMinFilter(filter);
    settings->setMagFilter(filter);
    _texture = std::make_shared<draw::Texture>(&engine);
    _texture->load(std::move(settings), std::move(image));
    _preview = ui.texturePreview(_texture);
}

void BakeLightmapPreview::draw(uint32_t width, uint32_t height)
{
    if (valid())
    {
        _preview->drawImage(width, height);
    }
}

void BakeLightmapPreview::clear()
{
    _preview.reset();
    _texture.reset();
}

bool BakeLightmapPreview::valid() const
{
    return _preview && _preview->valid();
}

}
