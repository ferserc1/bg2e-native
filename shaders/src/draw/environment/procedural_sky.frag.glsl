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
#include "common.glsl"
layout(push_constant) uniform Push
{
    GenerationPush params;
};
layout(location = 0) out vec4 outColor;
vec4 evaluate(vec2 pixel)
{
    vec3 n = cubeDirection(params.face, pixel / float(params.size));
    float altitude = max(n.y, 0.0);
    vec3 sky = mix(params.skyHorizonColor.rgb, params.skyZenithColor.rgb, sqrt(altitude));
    vec3 ground = params.groundColor.rgb;
    vec3 color = mix(ground, sky, smoothstep(-0.03, 0.03, n.y));
    if (params.sunRadiance.w > 0.5)
    {
        vec3 sun = params.sunDirectionRadius.xyz;
        float elevation = asin(clamp(sun.y, -1.0, 1.0));
        float day = smoothstep(-0.12, 0.18, elevation);
        float dusk = (1.0 - smoothstep(0.0, 0.35, abs(elevation))) * smoothstep(-0.18, 0.0, elevation);
        float sunward = pow(max(dot(n, sun), 0.0), 4.0);
        float lowSky = pow(1.0 - altitude, 3.0);
        // Authored colors remain the daytime reference. Night is a dim residual,
        // and twilight is an artistic approximation, not multiple scattering.
        vec3 warm = vec3(1.0, 0.32, 0.10);
        vec3 horizon = mix(params.skyHorizonColor.rgb, params.skyHorizonColor.rgb * warm, dusk * 0.85);
        sky = mix(horizon, params.skyZenithColor.rgb, sqrt(altitude));
        sky *= (0.002 + 0.998 * day) * params.solarParameters.x;
        sky += params.skyHorizonColor.rgb * warm * dusk * lowSky * (0.2 + 0.8 * sunward) * params.solarParameters.x;
        ground *= (0.002 + 0.998 * day) * params.solarParameters.x;
        color = mix(ground, sky, smoothstep(-0.03, 0.03, n.y));
        if (params.solarParameters.y > 0.5)
        {
            float radius = params.sunDirectionRadius.w;
            // Equal-area broadened disk for subpixel cubemap footprints. The
            // energy correction keeps a real-size sun from flickering away.
            float footprint = 1.5 / float(params.size);
            float filteredRadius = sqrt(radius * radius + footprint * footprint);
            float angle = acos(clamp(dot(n, sun), -1.0, 1.0));
            float edge = max(footprint * 0.5, 1e-6);
            float disk = 1.0 - smoothstep(max(filteredRadius - edge, 0.0), filteredRadius + edge, angle);
            float energy = (1.0 - cos(radius)) / max(1.0 - cos(filteredRadius), 1e-12);
            float horizonVisibility = smoothstep(-footprint * 0.5, footprint * 0.5, n.y);
            color += params.sunRadiance.rgb * disk * energy * horizonVisibility;
        }
    }
    else
    {
        vec3 sun = normalize(vec3(0.4, 0.7, 0.2));
        color += vec3(8.0, 6.5, 4.0) * pow(max(dot(n, sun), 0.0), 512.0);
    }
    return vec4(color, 1.0);
}
void main()
{
    outColor = evaluate(gl_FragCoord.xy);
}
