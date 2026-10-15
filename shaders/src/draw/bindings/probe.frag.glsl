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
#extension GL_GOOGLE_include_directive : require
#define DRAW_BINDINGS_FRAGMENT
#include "../lib/bindings.glsl"
layout(location = 0) in vec2 fragUV0;
layout(location = 1) in vec2 fragUV1;
layout(location = 0) out vec4 outColor;
void main()
{
    vec3 sum = vec3(0.0);
    for (uint slot = 0u; slot < 6u; ++slot)
    {
        sum += drawSampleMaterialLod(slot, fragUV0, fragUV1, 0.0).rgb;
    }
    sum += textureLod(samplerCube(drawSky, drawSkySampler), vec3(1, 0, 0), 0.0).rgb;
    sum += textureLod(samplerCube(drawIrradiance, drawIrradianceSampler), vec3(0, 1, 0), 0.0).rgb;
    sum += textureLod(samplerCube(drawPrefilter, drawPrefilterSampler), vec3(0, 0, 1), drawEnvironment.data.parameters.x).rgb;
    sum += textureLod(sampler2D(drawBRDF, drawBRDFSampler), vec2(0.5), 0.0).rgb;
    vec3 light = drawLights.data.count.x == 0u ? vec3(0.0) : drawLights.data.lights[0].color.rgb;
    sum += drawObject.data.material.albedo.rgb + light + drawFrame.data.cameraPosition.xyz * 0.01;
    outColor = vec4(sum / 12.0, 1.0);
}
