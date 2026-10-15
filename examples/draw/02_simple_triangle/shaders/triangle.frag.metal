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

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct PushConstants {
    float4 color;
};

fragment float4 fragMain(VertexOut in [[stage_in]],
                         constant PushConstants& pc [[buffer(0)]],
                         texture2d<float> uTex [[texture(0)]],
                         sampler uSampler [[sampler(1)]])
{
    float4 tex = uTex.sample(uSampler, in.uv);
    return float4(tex.rgb * pc.color.rgb, tex.a * pc.color.a);
}
