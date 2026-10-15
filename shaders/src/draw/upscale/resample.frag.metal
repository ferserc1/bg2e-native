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
fragment float4 fragMain(float4 position [[position]], texture2d<float> source [[texture(0)]],
    sampler sourceSampler [[sampler(0)]], constant float4& outputSize [[buffer(0)]])
{
    float2 size = float2(source.get_width(), source.get_height());
    float2 p = position.xy / outputSize.xy * size - 0.5;
    float2 b = floor(p);
    float2 w = p - b;
    float4 a = source.sample(sourceSampler, (b + 0.5) / size, level(0));
    float4 c = source.sample(sourceSampler, (b + float2(1.5, 0.5)) / size, level(0));
    float4 d = source.sample(sourceSampler, (b + float2(0.5, 1.5)) / size, level(0));
    float4 e = source.sample(sourceSampler, (b + 1.5) / size, level(0));
    return mix(mix(a, c, w.x), mix(d, e, w.x), w.y);
}
