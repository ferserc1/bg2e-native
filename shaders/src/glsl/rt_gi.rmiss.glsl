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
#extension GL_ARB_shading_language_include : require

layout(location = 0) rayPayloadInEXT GIPayload
{
    vec3 hitDirectLight;
    vec3 hitAlbedo;
    vec3 hitNormal;
    vec3 hitPosition;
    uint didHit;
}
payload;

void main()
{
    payload.hitDirectLight = vec3(0.0);
    payload.hitAlbedo = vec3(1.0);
    payload.hitNormal = vec3(0.0);
    payload.hitPosition = vec3(0.0);
    payload.didHit = 0u;
}
