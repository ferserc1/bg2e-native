/*
 *    business grade graphic engine (bg2e engine)
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
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_ray_tracing : require
#define RT_COUNT 128
#include "../lib/rt_probe.glsl"
layout(location=0) rayPayloadEXT vec4 color;
void main()
{
    uint pixel = gl_LaunchIDEXT.x;
    vec3 origin = vec3((float(pixel)+0.5)/32.0*0.8-0.4, -0.25, 0.0);
    color = vec4(1, 0, 1, 1);
    traceRayEXT(topLevel, 0u, 255u, 0u, 0u, 0u, origin, 0.001, vec3(0,0,1), 4.0, 0);
    imageStore(outputImage, ivec2(pixel, 0), color);
}
