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

#include "../lib/blue_noise.metal"
using namespace metal;
struct NoiseProbeData { uint4 metadata; uint4 sequence; };
kernel void compMain(uint2 outputPixel [[thread_position_in_grid]],
    constant NoiseProbeData& dataBlock [[buffer(1)]],
    texture2d<float,access::write> outputImage [[texture(0)]])
{
    if (outputPixel.x >= outputImage.get_width() || outputPixel.y >= outputImage.get_height())
    {
        return;
    }
    int2 pixel = int2(outputPixel) - int2(3,2);
    uint frame = dataBlock.sequence.x + outputPixel.y / dataBlock.metadata.y;
    float2 value = bg2eHashNoise2(pixel, frame, dataBlock.sequence.y);
    outputImage.write(float4(value,0,1), outputPixel);
}
