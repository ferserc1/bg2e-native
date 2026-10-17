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

// Primitive validation ABI; independent of engine material/scene policy.
struct ProbeVertex { vec4 position; vec4 normal; vec4 uv; vec4 tangent; };
struct ProbeInstance { mat4 model; mat4 normal; uvec4 ranges; uvec4 identity; };
struct ProbeMaterial { uvec4 images; vec4 parameters; };
layout(set=0, binding=0, rgba8) uniform image2D outputImage;
layout(set=0, binding=1) uniform texture2D images[RT_COUNT];
layout(set=0, binding=2) uniform sampler samplers[RT_COUNT];
layout(set=0, binding=3, std430) readonly buffer Vertices { ProbeVertex vertices[]; };
layout(set=0, binding=4, std430) readonly buffer Indices { uint indices[]; };
layout(set=0, binding=5, std430) readonly buffer Instances { ProbeInstance instances[]; };
layout(set=0, binding=6, std430) readonly buffer Materials { ProbeMaterial materials[]; };
layout(set=0, binding=7) uniform accelerationStructureEXT topLevel;
vec4 surface(uint instanceIndex, uint primitive, vec2 bary, uint pixel)
{
    ProbeInstance instance = instances[instanceIndex];
    ProbeMaterial material = materials[instance.ranges.w];
    uint first = instance.ranges.x + 3u * primitive;
    ProbeVertex a = vertices[instance.ranges.y + indices[first]];
    ProbeVertex b = vertices[instance.ranges.y + indices[first+1u]];
    ProbeVertex c = vertices[instance.ranges.y + indices[first+2u]];
    vec3 weights = vec3(1.0-bary.x-bary.y, bary);
    vec4 uv = a.uv*weights.x + b.uv*weights.y + c.uv*weights.z;
    uint imageIndex = material.images.x + pixel % material.images.y;
    uint samplerIndex = pixel % 3u == 0u ? 1u : 0u;
    vec2 selected = material.images.w == 0u ? uv.xy : uv.zw;
    return textureLod(sampler2D(images[nonuniformEXT(imageIndex)], samplers[nonuniformEXT(samplerIndex)]), selected, 0.0);
}
