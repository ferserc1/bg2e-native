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

constant float PI = 3.141592653589793;
struct GenerationPush {
    uint face;
    uint size;
    uint samples;
    uint sourceSize;
    float roughness;
    float sourceMaxLod;
    float padding0;
    float padding1;
    float4 skyHorizonColor;
    float4 skyZenithColor;
    float4 groundColor;
    float4 sunDirectionRadius;
    float4 sunRadiance;
    float4 solarParameters;
};
float3 cubeDirection(uint face, float2 uv)
{
    float2 p = uv * 2.0 - 1.0;
    if (face == 0u)
    {
        return normalize(float3(1.0, -p.y, -p.x));
    }
    if (face == 1u)
    {
        return normalize(float3(-1.0, -p.y, p.x));
    }
    if (face == 2u)
    {
        return normalize(float3(p.x, 1.0, p.y));
    }
    if (face == 3u)
    {
        return normalize(float3(p.x, -1.0, -p.y));
    }
    if (face == 4u)
    {
        return normalize(float3(p.x, -p.y, 1.0));
    }
    return normalize(float3(-p.x, -p.y, -1.0));
}
float2 hammersley(uint i, uint count)
{
    uint b = i;
    b = (b << 16u) | (b >> 16u);
    b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
    b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
    b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
    b = ((b & 0x00FF00FFu) << 8u) | ((b & 0xFF00FF00u) >> 8u);
    return float2(float(i) / float(count), float(b) * 2.3283064365386963e-10);
}
float3 tangentToWorld(float3 h, float3 n)
{
    float3 up = abs(n.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 t = normalize(cross(up, n));
    return normalize(t * h.x + cross(n, t) * h.y + n * h.z);
}
float3 sampleGGX(float2 xi, float3 n, float roughness)
{
    float a = max(roughness * roughness, 0.0001);
    float phi = 2.0 * PI * xi.x;
    float ct = sqrt((1.0 - xi.y) / max(1.0 + (a * a - 1.0) * xi.y, 0.000001));
    float st = sqrt(max(1.0 - ct * ct, 0.0));
    return tangentToWorld(float3(cos(phi) * st, sin(phi) * st, ct), n);
}
float geometryIBL(float nd, float roughness)
{
    float k = roughness * roughness * 0.5;
    return nd / max(nd * (1.0 - k) + k, 0.000001);
}
