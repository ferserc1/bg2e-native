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
struct Varyings {
    float4 position [[position]];
    float3 worldPosition;
    float3 worldNormal;
    uint2 identity [[flat]];
};
struct Output {
    float4 position [[color(0)]];
    float4 normal [[color(1)]];
    uint submesh [[color(2)]];
    uint chart [[color(3)]];
};
fragment Output fragMain(Varyings in [[stage_in]])
{
    if (dot(in.worldNormal,in.worldNormal) <= 1e-12)
    {
        discard_fragment();
    }
    Output out;
    out.position = float4(in.worldPosition,1.0);
    out.normal = float4(normalize(in.worldNormal),1.0);
    out.submesh = in.identity.x;
    out.chart = in.identity.y;
    return out;
}
