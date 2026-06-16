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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>
#include <bg2e/gpu/Image.hpp>

#include <memory>
#include <string>

namespace bg2e {
namespace gpu {

class Device;

class BG2E_API CubeMap : public DeviceResource {
public:
    explicit CubeMap(Device* device);
    virtual ~CubeMap() = default;

    void create(const CubeMapDescription& description);

    Image*       image()       { return _image.get(); }
    const Image* image() const { return _image.get(); }

    uint32_t    size()       const { return _size; }
    uint32_t    mipLevels()  const { return _mipLevels; }
    PixelFormat pixelFormat() const { return _format; }

    bool isValid() const override;
    void cleanup() override;

protected:
    std::shared_ptr<gpu::Image> _image;
    uint32_t    _size      = 0;
    uint32_t    _mipLevels = 1;
    PixelFormat _format    = PixelFormat::Undefined;
};

}
}
