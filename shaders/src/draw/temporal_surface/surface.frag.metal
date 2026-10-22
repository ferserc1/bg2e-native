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

#include "../raster/vertex.metal"
struct TemporalSurfaceOut {
    uint token [[color(0)]];
};
fragment TemporalSurfaceOut fragMain(RasterVaryings in [[stage_in]], constant DrawObjectData& drawObject [[buffer(7)]],
    texture2d<float> drawAlbedo [[texture(0)]], sampler drawAlbedoSampler [[sampler(0)]],
    constant uint4& temporalIdentity [[buffer(8)]])
{
    DrawMaterialData m = drawObject.material;
    float4 a = drawSampleMaterial(drawAlbedo, drawAlbedoSampler, m, 0u, in.uv0, in.uv1) * m.albedo;
    uint flags = m.uvSets[1].z;
    if (((flags & 2u) != 0u && a.a < m.emissionAlpha.y) || ((flags & 4u) != 0u && a.a <= 0.0))
    {
        discard_fragment();
    }
    TemporalSurfaceOut out;
    out.token = temporalIdentity.y != 0u ? temporalIdentity.x : 0u;
    return out;
}
