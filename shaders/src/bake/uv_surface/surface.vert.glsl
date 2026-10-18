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

#version 450
struct UvVertex { vec4 position; vec4 normal; vec4 uv; uvec4 identity; };
layout(set=0,binding=0,std140) uniform Transform { mat4 model; mat4 normal; } transform;
layout(set=0,binding=1,std430) readonly buffer Vertices { UvVertex values[]; } vertices;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec3 worldNormal;
layout(location=2) flat out uvec2 identity;
void main()
{
    UvVertex item = vertices.values[gl_VertexIndex];
    // Both native viewport conventions map clip +Y to top: v=0 is row zero.
    gl_Position = vec4(item.uv.x*2.0-1.0,1.0-item.uv.y*2.0,0.0,1.0);
    worldPosition = (transform.model*item.position).xyz;
    worldNormal = normalize(mat3(transform.normal)*item.normal.xyz);
    identity = item.identity.xy;
}
