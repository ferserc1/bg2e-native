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

#version 450
layout(set=0,binding=0) uniform texture2D sourceImage;
layout(set=0,binding=1) uniform sampler sourceSampler;
layout(location=0) out vec4 outputColor;
void main()
{
    vec2 uv = gl_FragCoord.xy/vec2(textureSize(sampler2D(sourceImage,sourceSampler),0));
    outputColor = texture(sampler2D(sourceImage,sourceSampler),uv);
}
