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

#include <bg2e/gpu/metal/SurfaceFrame.hpp>
#include <bg2e/gpu/metal/Image.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

gpu::Image* SurfaceFrame::colorImage() const
{
    if (_colorImage) return _colorImage.get();
    return _colorImageRaw;
}

bool SurfaceFrame::isValid() const
{
    return _colorImage != nullptr || _colorImageRaw != nullptr;
}

void SurfaceFrame::setColorImage(std::unique_ptr<metal::Image> img)
{
    _colorImage = std::move(img);
}

#else

gpu::Image* SurfaceFrame::colorImage() const
{
    return nullptr;
}

bool SurfaceFrame::isValid() const
{
    return false;
}

void SurfaceFrame::setColorImage(std::unique_ptr<metal::Image>)
{
}

#endif

}
}
}