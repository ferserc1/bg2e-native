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

#include <metal_stdlib>
using namespace metal;
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    texture2d<uint,access::read> chartImage [[texture(0)]], sampler nearestSampler [[sampler(0)]],
    texture2d<float,access::write> coverageImage [[texture(1)]])
{
    if (pixel.x >= coverageImage.get_width() || pixel.y >= coverageImage.get_height())
    {
        return;
    }
    coverageImage.write(float4(chartImage.read(pixel).r != 0u ? 1.0 : 0.0),pixel);
}
