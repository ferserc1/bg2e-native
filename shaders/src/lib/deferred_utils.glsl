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

vec3 reconstructWorldPosition(
    vec2 uv,
    float depth,
    mat4 inverseViewProjection
) {
    // We invert the V coordinate because in Vulkan, screen coordinates
    // are vertically inverted. We need to perform vertical sampling
    // because, to reconstruct the position using the inverse viewProjection matrix,
    // those coordinates will be screen coordinates
    vec2 ndc = vec2(
        uv.x * 2.0 - 1.0,
        (1.0 - uv.y) * 2.0 - 1.0
    );
    vec4 clipPosition = vec4(
        ndc.x,
        ndc.y,
        depth,
        1.0
    );

    vec4 worldPosition = inverseViewProjection * clipPosition;
    worldPosition /= worldPosition.w;

    return worldPosition.xyz;
}