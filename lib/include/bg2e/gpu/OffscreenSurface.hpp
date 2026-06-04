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

#include <bg2e/gpu/Surface.hpp>

namespace bg2e {
namespace gpu {

class BG2E_API OffscreenSurface : public Surface {
public:
    OffscreenSurface(uint32_t width, uint32_t height)
    {
        _width  = width;
        _height = height;
    }

    bool isOffscreen() const override { return true; }
    bool isValid()     const override { return true; }
    uint32_t width()   const override { return _width;  }
    uint32_t height()  const override { return _height; }
};

}
}
