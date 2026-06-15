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

#ifndef RT_MATERIAL_DATA_GLSL
#define RT_MATERIAL_DATA_GLSL

#define MAX_RT_OBJECTS 256

// Must match C++ RTMaterialData struct layout (32 bytes)
struct RTMaterialData {
    vec4 albedo;          // base::Color (rgba)
    vec2 albedoScale;
    uint indexOffset;     // firstIndex of the submesh in the shared index buffer
    uint padding1;
};

// Must match C++ geo::Vertex / VertexPNUUT struct layout (52 bytes)
struct RTVertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord0;
    vec2 texCoord1;
    vec3 tangent;
};

#endif