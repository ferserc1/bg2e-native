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

// Basic (Lambert-only) lighting used by the ray traced reflection pass.
// No specular, sheen or any other term: reflections only need a cheap
// diffuse contribution from the lights flagged as "affects reflections".

#ifndef BASIC_LIGHTING_GLSL
#define BASIC_LIGHTING_GLSL

#include "uniforms.glsl"

float basicAttenuation(vec3 lightPosition, vec3 fragPosition)
{
    float dist = length(lightPosition - fragPosition);
    return 1.0 / (dist * dist);
}

vec3 basicLightingDirectional(LightData light, vec3 normal, vec3 albedo)
{
    vec3 L = normalize(-light.direction);
    float NdotL = max(dot(normal, L), 0.0);
    vec3 radiance = light.color.rgb * light.intensity;
    return albedo * radiance * NdotL;
}

vec3 basicLightingPoint(LightData light, vec3 fragPos, vec3 normal, vec3 albedo)
{
    vec3 L = normalize(light.position - fragPos);
    float NdotL = max(dot(normal, L), 0.0);
    float attenuation = basicAttenuation(light.position, fragPos);
    vec3 radiance = light.color.rgb * light.intensity * attenuation;
    return albedo * radiance * NdotL;
}

vec3 basicLightingSpot(LightData light, vec3 fragPos, vec3 normal, vec3 albedo)
{
    vec3 L = normalize(light.position - fragPos);
    float NdotL = max(dot(normal, L), 0.0);
    float attenuation = basicAttenuation(light.position, fragPos);

    // Spot cone falloff (same convention as the PBR path)
    vec3 spotDir = normalize(-light.direction);
    float theta = dot(L, spotDir);
    float cosSpotAngle = cos(radians(light.spotAngle));
    float cosSpotCutoff = cos(radians(light.spotCutoff));
    float spotFactor;
    if (theta > cosSpotCutoff)
    {
        spotFactor = 1.0;
    }
    else if (theta < cosSpotAngle)
    {
        spotFactor = 0.0;
    }
    else
    {
        spotFactor = smoothstep(cosSpotAngle, cosSpotCutoff, theta);
    }
    attenuation *= spotFactor;

    vec3 radiance = light.color.rgb * light.intensity * attenuation;
    return albedo * radiance * NdotL;
}

// Main dispatcher: routes to the proper light type processing function.
vec3 computeBasicLighting(LightData light, vec3 fragPos, vec3 normal, vec3 albedo)
{
    if (light.type == LIGHT_TYPE_POINT)
    {
        return basicLightingPoint(light, fragPos, normal, albedo);
    }
    else if (light.type == LIGHT_TYPE_SPOT)
    {
        return basicLightingSpot(light, fragPos, normal, albedo);
    }
    else if (light.type == LIGHT_TYPE_DIRECTIONAL)
    {
        return basicLightingDirectional(light, normal, albedo);
    }
    return vec3(0.0);
}

#endif
