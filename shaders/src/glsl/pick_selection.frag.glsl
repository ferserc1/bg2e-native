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

layout (location = 0) out vec4 outColor;

layout (location = 0) in flat uint inIdentifier;

void main()
{
    const float inv255 = 1.0 / 255.0;

    uvec4 bytes = uvec4(
        inIdentifier & 0xFFu,
        (inIdentifier >> 8)  & 0xFFu,
        (inIdentifier >> 16) & 0xFFu,
        (inIdentifier >> 24) & 0xFFu
    );

    outColor = vec4(bytes) * inv255;
}
