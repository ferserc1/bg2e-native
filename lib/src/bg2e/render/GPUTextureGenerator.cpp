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

#include <bg2e/render/GPUTextureGenerator.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e::render {

GPUTextureGenerator::GPUTextureGenerator(Engine * engine, uint32_t w, uint32_t h)
    :_engine{engine}, _width{w}, _height{h}
{

}

vulkan::Image * GPUTextureGenerator::createImage(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage)
{
    return vulkan::Image::createAllocatedImage(
        _engine,
        format,
        extent,
        usage
    );
}


Texture * GPUTextureGenerator::wrapImage(
    vulkan::Image * image,
    bool useMipmaps,
    base::Texture::Filter magFilter,
    base::Texture::Filter minFilter
) {
    auto baseTexture = new base::Texture();
    baseTexture->setUseMipmaps(useMipmaps);
    baseTexture->setMagFilter(magFilter);
    baseTexture->setMinFilter(minFilter);
    auto result = new Texture(_engine);
    result->load(baseTexture, image);
    return result;
}

}
