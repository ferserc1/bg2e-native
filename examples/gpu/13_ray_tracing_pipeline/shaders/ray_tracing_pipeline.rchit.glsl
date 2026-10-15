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

#version 460
#extension GL_EXT_ray_tracing : enable

struct HitInfo {
    vec3 hitPos;
    vec3 normal;
    vec3 albedo;
    vec3 emission;
    float roughness;
    int hit;
};
layout(location = 0) rayPayloadInEXT HitInfo payload;
hitAttributeEXT vec2 attribs;

// Mirror of the C++ InstanceData struct.
//   shapeType (emission.a): 0 = plane, 1 = sphere, 2 = cube
struct InstanceData {
    vec4 albedo;         // rgb, a = roughness
    vec4 emission;       // rgb, a = shapeType
    vec4 normalOrCenter; // plane: world normal; sphere/cube: world center; w = sphere radius
    vec4 extents;        // cube: world-space half-extents
};
layout(set = 2, binding = 0) buffer InstanceBuffer
{
    InstanceData data[];
}
instances;

void main()
{
    InstanceData inst = instances.data[gl_InstanceCustomIndexEXT];
    int shapeType = int(inst.emission.a);

    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    // Analytic surface normal (the scene is planes / a sphere / an axis-aligned
    // cube, all without rotation or scale beyond what is baked into the data).
    vec3 n;
    if (shapeType == 0) // plane
    {
        n = normalize(inst.normalOrCenter.xyz);
    }
    else if (shapeType == 1) // sphere
    {
        n = normalize(worldPos - inst.normalOrCenter.xyz);
    }
    else // cube: dominant axis of the local position
    {
        vec3 local = (worldPos - inst.normalOrCenter.xyz) / max(inst.extents.xyz, vec3(1e-4));
        vec3 a = abs(local);
        if (a.x > a.y && a.x > a.z)
        {
            n = vec3(sign(local.x), 0.0, 0.0);
        }
        else if (a.y > a.z)
        {
            n = vec3(0.0, sign(local.y), 0.0);
        }
        else
        {
            n = vec3(0.0, 0.0, sign(local.z));
        }
    }

    // Make the normal face the incoming ray (two-sided surfaces).
    if (dot(n, gl_WorldRayDirectionEXT) > 0.0)
    {
        n = -n;
    }

    payload.hitPos = worldPos;
    payload.normal = n;
    payload.albedo = inst.albedo.rgb;
    payload.emission = inst.emission.rgb;
    payload.roughness = inst.albedo.a;
    payload.hit = 1;
}
