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
layout(location=0) in vec3 worldPosition;
layout(location=1) in vec3 worldNormal;
layout(location=2) flat in uvec2 identity;
layout(location=0) out vec4 outputPosition;
layout(location=1) out vec4 outputNormal;
layout(location=2) out uint outputSubmesh;
layout(location=3) out uint outputChart;
void main()
{
    if (dot(worldNormal,worldNormal) <= 1e-12)
    {
        discard;
    }
    outputPosition = vec4(worldPosition,1.0);
    outputNormal = vec4(normalize(worldNormal),1.0);
    outputSubmesh = identity.x;
    outputChart = identity.y;
}
