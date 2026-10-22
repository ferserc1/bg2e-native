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

#include "surface_data_v2.metal"
vertex TemporalSurfaceVaryings vertMain(RasterVertex in [[stage_in]], DRAW_VERTEX_DATA_ARGUMENTS,
    constant TemporalSurfaceUniforms& temporalSurface [[buffer(9)]])
{
    TemporalSurfaceVaryings out;
    out.worldPosition = (drawObject.model * float4(in.position, 1.0)).xyz;
    out.worldNormal = normalize(float3x3(drawObject.normal[0].xyz, drawObject.normal[1].xyz, drawObject.normal[2].xyz) * in.normal);
    float3 t = float3x3(drawObject.model[0].xyz, drawObject.model[1].xyz, drawObject.model[2].xyz) * in.tangent;
    t -= out.worldNormal * dot(t, out.worldNormal);
    if (dot(t, t) < 1e-8)
    {
        t = cross(abs(out.worldNormal.z) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0), out.worldNormal);
    }
    out.worldTangent = normalize(t);
    out.tangentSign = determinant(float3x3(drawObject.model[0].xyz, drawObject.model[1].xyz, drawObject.model[2].xyz)) < 0.0 ? -1.0 : 1.0;
    out.uv0 = in.texcoord0;
    out.uv1 = in.texcoord1;
    float4 previousWorld = temporalSurface.previousModel * float4(in.position, 1.0);
    out.previousWorldPosition = previousWorld.xyz;
    out.currentClip = temporalSurface.currentViewProjection * float4(out.worldPosition, 1.0);
    out.previousClip = temporalSurface.previousViewProjection * previousWorld;
    out.position = drawFrame.projection * drawFrame.view * float4(out.worldPosition, 1.0);
    return out;
}
