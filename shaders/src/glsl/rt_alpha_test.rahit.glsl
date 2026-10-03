/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 */

#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require

#include "lib/ray_tracing.glsl"
#include "lib/rt_material_data.glsl"

hitAttributeEXT vec2 attribs;

void main()
{
    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    if (!rtAlphaTestHit(gl_InstanceCustomIndexEXT, uint(gl_PrimitiveID), bary))
    {
        ignoreIntersectionEXT;
    }
}
