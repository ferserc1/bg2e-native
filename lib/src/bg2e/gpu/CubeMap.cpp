/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/Device.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {

CubeMap::CubeMap(Device* device)
    : DeviceResource(device)
{
}

void CubeMap::create(const CubeMapDescription& description)
{
    if (description.size == 0)
    {
        throw std::runtime_error("gpu::CubeMap::create: size must be > 0");
    }
    if (description.mipLevels < 1)
    {
        throw std::runtime_error("gpu::CubeMap::create: mipLevels must be >= 1");
    }
    if (!hasFlag(description.usage, ImageUsage::Sampled))
    {
        throw std::runtime_error("gpu::CubeMap::create: usage must include ImageUsage::Sampled");
    }
    if (!hasFlag(description.usage, ImageUsage::ColorAttachment))
    {
        throw std::runtime_error("gpu::CubeMap::create: usage must include ImageUsage::ColorAttachment");
    }

    _size      = description.size;
    _mipLevels = description.mipLevels;
    _format    = description.format;

    _image = device()->createImage(ImageDescription{
        .size      = { description.size, description.size },
        .format    = description.format,
        .usage     = description.usage,
        .type      = ImageType::Cubemap,
        .mipLevels = description.mipLevels,
        .debugName = description.debugName
    });
}

bool CubeMap::isValid() const
{
    return _image && _image->isValid();
}

void CubeMap::cleanup()
{
    if (_image)
    {
        _image->cleanup();
        _image.reset();
    }
    _size      = 0;
    _mipLevels = 1;
    _format    = PixelFormat::Undefined;
}

}
}
