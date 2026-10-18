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

#include <bg2e/bake/LightmapPixels.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace bg2e::bake {
size_t LightmapPixels::elementCount() const
{
    const auto limit = std::numeric_limits<size_t>::max();
    if (!width || !height)
    {
        throw std::invalid_argument("Lightmap pixels require nonzero dimensions");
    }
    if (size_t(width) > limit/height || size_t(width)*height > limit/3)
    {
        throw std::length_error("Lightmap RGB dimensions overflow element addressing");
    }
    return size_t(width)*height*3;
}

void LightmapPixels::validate() const
{
    const auto count = elementCount();
    if (format == LightmapPixelFormat::RGB8)
    {
        const auto* data = std::get_if<std::vector<uint8_t>>(&rgb);
        if (!data || data->size() != count)
        {
            throw std::invalid_argument("RGB8 lightmap needs exactly width*height*3 bytes");
        }
    }
    else if (format == LightmapPixelFormat::RGB32F)
    {
        if (count > std::numeric_limits<size_t>::max()/sizeof(float))
        {
            throw std::length_error("RGB32F lightmap byte addressing overflows");
        }
        const auto* data = std::get_if<std::vector<float>>(&rgb);
        if (!data || data->size() != count)
        {
            throw std::invalid_argument("RGB32F lightmap needs exactly width*height*3 floats");
        }
        for (auto value : *data)
        {
            if (!std::isfinite(value) || value < 0)
            {
                throw std::invalid_argument("Lightmap multipliers must be finite and nonnegative");
            }
        }
    }
    else
    {
        throw std::invalid_argument("Unknown lightmap pixel format");
    }
}
}
