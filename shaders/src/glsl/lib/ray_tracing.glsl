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

#ifndef RAY_TRACING_GLSL
#define RAY_TRACING_GLSL

#include "uniforms.glsl"

float _shadowHash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec3 _shadowRandomDirection(vec3 normal, vec2 seed, float angle)
{
    float r1 = _shadowHash(seed);
    float r2 = _shadowHash(seed + vec2(17.3, 53.1));

    float phi = 6.2831853 * r1;
    float cosTheta = 1.0 - r2 * (1.0 - cos(angle));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return normalize(
        tangent * cos(phi) * sinTheta +
        bitangent * sin(phi) * sinTheta +
        normal * cosTheta
    );
}

bool _shadowRayTest(
    accelerationStructureEXT tlas,
    vec3 origin, vec3 dir, float tMax
) {
    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, tlas,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF, origin, 0.001, dir, tMax);
    while (rayQueryProceedEXT(rq)) {}

    return rayQueryGetIntersectionTypeEXT(rq, true) ==
           gl_RayQueryCommittedIntersectionNoneEXT;
}

float queryShadow(
    accelerationStructureEXT tlas,
    vec3 worldPos, vec3 normal,
    LightData light
) {
    vec3 toLight;
    float tMax;
    if (light.type == LIGHT_TYPE_POINT) {
        toLight = light.position - worldPos;
        tMax = length(toLight);
        toLight = normalize(toLight);
    } else if (light.type == LIGHT_TYPE_DIRECTIONAL) {
        toLight = -normalize(light.direction);
        tMax = 1000000.0;
    } else {
        toLight = light.position - worldPos;
        tMax = length(toLight);
        toLight = normalize(toLight);
    }

    vec3 origin = worldPos + normal * 0.01;

    int samples = light.shadowSamples;
    if (samples <= 1)
    {
        return _shadowRayTest(tlas, origin, toLight, tMax) ? 1.0 : 0.0;
    }

    float angle;
    if (light.type == LIGHT_TYPE_DIRECTIONAL) {
        angle = light.sourceSize * 3.14159265 / 180.0;
    } else {
        angle = atan(light.sourceSize, tMax);
    }

    int occluded = 0;
    for (int i = 0; i < samples; i++)
    {
        vec2 seed = worldPos.xz * 1000.0 + vec2(float(i) * 13.7, float(i) * 7.3);
        vec3 dir = _shadowRandomDirection(toLight, seed, angle);
        if (!_shadowRayTest(tlas, origin, dir, tMax))
        {
            occluded++;
        }
    }

    return 1.0 - float(occluded) / float(samples);
}

bool queryAO(
    accelerationStructureEXT tlas,
    vec3 worldPos,
    vec3 normal,
    vec3 rayDir,
    float radius,
    float bias,
    out float hitDistance
) {
    rayQueryEXT rq;

    rayQueryInitializeEXT(
        rq,
        tlas,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        worldPos + normal * bias,
        0.001,
        rayDir,
        radius
    );

    while (rayQueryProceedEXT(rq)) {}

    bool hit = rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionNoneEXT;

    if (hit)
    {
        hitDistance = rayQueryGetIntersectionTEXT(rq, true);
    }
    else
    {
        hitDistance = radius;
    }
    return hit;
}

#endif
