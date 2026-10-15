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

kernel void compMain(texture2d<float, access::read> inputImage [[texture(0)]],
                     texture2d<float, access::write> outputImage [[texture(1)]],
                     uint2 gid [[thread_position_in_grid]])
{
    uint w = inputImage.get_width();
    uint h = inputImage.get_height();
    if (gid.x >= w || gid.y >= h)
    {
        return;
    }

    int2 pos = int2(gid);

    // Sample 3x3 neighborhood luminances
    float tl = dot(inputImage.read(uint2(pos + int2(-1, -1))).rgb, float3(0.299, 0.587, 0.114));
    float t = dot(inputImage.read(uint2(pos + int2(0, -1))).rgb, float3(0.299, 0.587, 0.114));
    float tr = dot(inputImage.read(uint2(pos + int2(1, -1))).rgb, float3(0.299, 0.587, 0.114));
    float l = dot(inputImage.read(uint2(pos + int2(-1, 0))).rgb, float3(0.299, 0.587, 0.114));
    float r = dot(inputImage.read(uint2(pos + int2(1, 0))).rgb, float3(0.299, 0.587, 0.114));
    float bl = dot(inputImage.read(uint2(pos + int2(-1, 1))).rgb, float3(0.299, 0.587, 0.114));
    float b = dot(inputImage.read(uint2(pos + int2(0, 1))).rgb, float3(0.299, 0.587, 0.114));
    float br = dot(inputImage.read(uint2(pos + int2(1, 1))).rgb, float3(0.299, 0.587, 0.114));

    // Sobel kernels
    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    edge = clamp(edge, 0.0f, 1.0f);

    outputImage.write(float4(float3(edge), 1.0), gid);
}
