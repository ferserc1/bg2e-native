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
#extension GL_EXT_ray_query : require

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec3 vAlbedo;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform LightUBO
{
    vec4 lightPosition; // xyz = world position
    vec4 lightColor;    // rgb = color, a = intensity
    vec4 ambient;       // rgb = ambient term
}
light;

layout(set = 2, binding = 1) uniform accelerationStructureEXT topLevelAS;

void main()
{
    vec3 N = normalize(vWorldNormal);

    vec3 toLight = light.lightPosition.xyz - vWorldPos;
    float distance = length(toLight);
    vec3 lightDir = toLight / distance;
    float ndotl = max(dot(N, lightDir), 0.0);

    // Cast a visibility ray from the shaded point toward the light. If anything
    // is hit before the light, the point is in shadow.
    const float bias = 0.02;
    float shadow = 1.0;

    rayQueryEXT rq;
    rayQueryInitializeEXT(
        rq,
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        vWorldPos + N * bias,
        bias,
        lightDir,
        distance - bias);
    rayQueryProceedEXT(rq);
    if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        shadow = 0.15;
    }

    vec3 diffuse = vAlbedo * light.lightColor.rgb * light.lightColor.a * ndotl * shadow;
    vec3 ambient = vAlbedo * light.ambient.rgb;

    outColor = vec4(ambient + diffuse, 1.0);
}
