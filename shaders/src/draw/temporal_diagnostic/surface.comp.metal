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
static_assert(sizeof(DiagnosticUniforms) == 16, "Temporal surface display uniform size");
kernel void compMain(uint2 pixel [[thread_position_in_grid]],
    constant DiagnosticUniforms& dataBlock [[buffer(1)]],
    texture2d<uint> tokenImage [[texture(0)]],
    texture2d<float> surfaceImage [[texture(1)]],
    texture2d<float,access::write> outputImage [[texture(2)]])
{
    if (any(pixel >= dataBlock.options.xy))
    {
        return;
    }
    uint token = tokenImage.read(pixel).r;
    float4 value = surfaceImage.read(pixel);
    float3 color = float3(0);
    if (token != 0u)
    {
        if (dataBlock.options.z == 3u)
        {
            if (all(isfinite(value.xy)))
            {
                color = float3(clamp(float2(0.5)+value.xy/64.0,float2(0),float2(1)),0.5);
            }
        }
        else if (value.w > 0.5 && all(isfinite(value)))
        {
            color = dataBlock.options.z == 4u ?
                0.5+0.5*value.xyz/(float3(1)+abs(value.xyz)) :
                clamp(0.5+0.5*value.xyz,float3(0),float3(1));
        }
    }
    outputImage.write(float4(color,1),pixel);
}
