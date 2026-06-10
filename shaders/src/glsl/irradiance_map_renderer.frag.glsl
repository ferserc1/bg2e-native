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
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (location = 0) in vec3 fragNormal;
layout (location = 1) in flat int inCurrentMipLevel;
layout (location = 2) in flat int inTotalMipLevels;
layout (location = 3) in flat int inFaceIndex;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform samplerCube skyTexture;

void main()
{
    // Compute the irradiance using the pre-filtered environment map
    const float sampleDelta = 0.025;
    const float PI = 3.14159265359;
    vec3 normal = normalize(fragNormal);
    vec3 irradiance = vec3(0.0);
    vec3 maxRange = vec3(40.0);
    
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up = cross(normal,right);

    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // Spherical to cartesian
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

            // tangent space to world space
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            vec3 tex = texture(skyTexture, sampleVec).rgb;
            irradiance += clamp(tex, vec3(0.0), maxRange) * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    outFragColor = vec4(irradiance, 1.0);
}
