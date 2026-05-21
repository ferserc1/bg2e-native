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

bool queryShadow(
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

    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, tlas,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF, worldPos + normal * 0.01, 0.001, toLight, tMax);
    while (rayQueryProceedEXT(rq)) {}

    return rayQueryGetIntersectionTypeEXT(rq, true) ==
           gl_RayQueryCommittedIntersectionNoneEXT;
}

#endif
