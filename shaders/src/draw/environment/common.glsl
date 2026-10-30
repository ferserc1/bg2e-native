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

const float PI = 3.141592653589793;
struct GenerationPush {
    uint face;
    uint size;
    uint samples;
    uint sourceSize;
    float roughness;
    float sourceMaxLod;
    float padding0;
    float padding1;
    vec4 skyHorizonColor;
    vec4 skyZenithColor;
    vec4 groundColor;
    vec4 sunDirectionRadius;
    vec4 sunRadiance;
    vec4 solarParameters;
};
vec3 cubeDirection(uint face, vec2 uv)
{
    vec2 p = uv * 2.0 - 1.0;
    if (face == 0u)
    {
        return normalize(vec3(1.0, -p.y, -p.x));
    }
    if (face == 1u)
    {
        return normalize(vec3(-1.0, -p.y, p.x));
    }
    if (face == 2u)
    {
        return normalize(vec3(p.x, 1.0, p.y));
    }
    if (face == 3u)
    {
        return normalize(vec3(p.x, -1.0, -p.y));
    }
    if (face == 4u)
    {
        return normalize(vec3(p.x, -p.y, 1.0));
    }
    return normalize(vec3(-p.x, -p.y, -1.0));
}
vec2 hammersley(uint i, uint count)
{
    uint b = i;
    b = (b << 16u) | (b >> 16u);
    b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
    b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
    b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
    b = ((b & 0x00FF00FFu) << 8u) | ((b & 0xFF00FF00u) >> 8u);
    return vec2(float(i) / float(count), float(b) * 2.3283064365386963e-10);
}
vec3 tangentToWorld(vec3 h, vec3 n)
{
    vec3 up = abs(n.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 t = normalize(cross(up, n));
    return normalize(t * h.x + cross(n, t) * h.y + n * h.z);
}
vec3 sampleGGX(vec2 xi, vec3 n, float roughness)
{
    float a = max(roughness * roughness, 0.0001);
    float phi = 2.0 * PI * xi.x;
    float ct = sqrt((1.0 - xi.y) / max(1.0 + (a * a - 1.0) * xi.y, 0.000001));
    float st = sqrt(max(1.0 - ct * ct, 0.0));
    return tangentToWorld(vec3(cos(phi) * st, sin(phi) * st, ct), n);
}
float geometryIBL(float nd, float roughness)
{
    float k = roughness * roughness * 0.5;
    return nd / max(nd * (1.0 - k) + k, 0.000001);
}
