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

#include <metal_stdlib>
using namespace metal;
struct DiagnosticUniforms { uint4 options; };
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant DiagnosticUniforms& dataBlock [[buffer(1)]],
    texture2d<uint> tokenImage [[texture(0)]],
    texture2d<float> acceptanceImage [[texture(1)]],
    texture2d<float,access::write> outputImage [[texture(2)]])
{
    if (any(pixel >= dataBlock.options.xy))
    {
        return;
    }
    uint token = tokenImage.read(pixel).r;
    float3 color = float3(1,0.5,0);
    if (dataBlock.options.w != 0u)
    {
        float2 uv = (float2(pixel)+0.5)/float2(dataBlock.options.xy);
        uint2 size = uint2(acceptanceImage.get_width(),acceptanceImage.get_height());
        uint2 source = min(uint2(uv*float2(size)),size-1);
        float4 metrics = acceptanceImage.read(source);
        uint channel = min(dataBlock.options.z-6u,3u);
        float value = metrics[channel];
        color = token != 0u && isfinite(value) ? float3(clamp(value,0.0,1.0)) : float3(0);
    }
    outputImage.write(float4(color,1),pixel);
}
