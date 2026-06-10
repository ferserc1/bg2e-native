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

#include "lib/constants.glsl"
#include "lib/uniforms.glsl"

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    // Use the Schlick approximation for fresnel with roughness
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 normal, vec3 halfVector, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(normal, halfVector), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;

    return a2 / (PI * denom * denom);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness)
{
    float NdotV = max(dot(normal, viewDir), 0.0);
    float NdotL = max(dot(normal, lightDir), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float calcAttenuation(vec3 lightPosition, vec3 fragPosition)
{
    float distance = length(lightPosition - fragPosition);
    return 1.0 / (distance * distance);
}

vec3 calcRadiancePoint(
    LightData light,
    vec3 viewDir,
    vec3 fragPos,
    float metallic,
    float roughness,
    vec3 F0,
    vec3 normal,
    vec3 albedo,
    float sheenIntensity,
    vec3 sheenColor,
    float ambientOcclussion
) {
    // calculate per-light radiance
    vec3 L = normalize(light.position - fragPos);
    vec3 H = normalize(viewDir + L);
    float attenuation    = calcAttenuation(light.position, fragPos);
    vec3 radiance     = light.color.rgb * light.intensity * attenuation;
    
    // cook-torrance brdf
    float NDF = distributionGGX(normal, H, roughness);
    float G   = geometrySmith(normal, viewDir, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(normal, L), 0.0);
    vec3 base = (kD * albedo / PI + specular) * radiance * NdotL;
    
    vec3 sheen = calcSheen(normal, viewDir, sheenColor, sheenIntensity) * ambientOcclussion;
    
    return base + sheen;
}

vec3 calcRadianceSpot(LightData light, vec3 viewDir, vec3 fragPos, float metallic, float roughness, vec3 F0, vec3 normal, vec3 albedo, float sheenIntensity, vec3 sheenColor, float ambientOcclussion)
{
    float spotAngle = light.spotAngle;
    float spotCutoff = light.spotCutoff;
    vec3 L = normalize(light.position - fragPos);
    vec3 H = normalize(viewDir + L);
    float attenuation = calcAttenuation(light.position, fragPos);

    // Spot attenuation based on angle between light direction and fragment direction
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

    // cook-torrance brdf
    float NDF = distributionGGX(normal, H, roughness);
    float G   = geometrySmith(normal, viewDir, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(normal, L), 0.0);
    vec3 base = (kD * albedo / PI + specular) * radiance * NdotL;
    vec3 sheen = calcSheen(normal, viewDir, sheenColor, sheenIntensity) * ambientOcclussion;
    return base + sheen;
}

vec3 calcRadianceDirectional(LightData light, vec3 viewDir, vec3 fragPos, float metallic, float roughness, vec3 F0, vec3 normal, vec3 albedo, float sheenIntensity, vec3 sheenColor, float ambientOcclussion)
{
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(viewDir + L);
    vec3 radiance     = light.color.rgb * light.intensity;

    // cook-torrance brdf
    float NDF = distributionGGX(normal, H, roughness);
    float G   = geometrySmith(normal, viewDir, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(normal, L), 0.0);
    vec3 base = (kD * albedo / PI + specular) * radiance * NdotL;
    vec3 sheen = calcSheen(normal, viewDir, sheenColor, sheenIntensity) * ambientOcclussion;
    return base + sheen;
}

vec3 calcRadiance(
    LightData light,
    vec3 viewDir,
    vec3 fragPos,
    float metallic,
    float roughness,
    vec3 F0,
    vec3 normal,
    vec3 albedo,
    float sheenIntensity,
    vec3 sheenColor,
    float ambientOcclussion
) {
    if (light.type == LIGHT_TYPE_POINT)
    {
        return calcRadiancePoint(light, viewDir, fragPos, metallic, roughness, F0, normal, albedo, sheenIntensity, sheenColor, ambientOcclussion);
    }
    else if (light.type == LIGHT_TYPE_SPOT)
    {
        return calcRadianceSpot(light, viewDir, fragPos, metallic, roughness, F0, normal, albedo, sheenIntensity, sheenColor, ambientOcclussion);
    }
    else if (light.type == LIGHT_TYPE_DIRECTIONAL)
    {
        return calcRadianceDirectional(light, viewDir, fragPos, metallic, roughness, F0, normal, albedo, sheenIntensity, sheenColor, ambientOcclussion);
    }
    return vec3(0.0);
}

vec3 calcAmbientLight(
    vec3 viewDir,
    vec3 normal,
    vec3 F0,
    vec3 albedo,
    float metallic,
    float roughness,
    samplerCube irradianceMap,
    samplerCube prefilteredEnvMap,
    float maxLOD,
    sampler2D brdfLUT,
    float ambientOcclussion,
    float sheenIntensity,
    vec3 sheenColor
) {
    vec3 R = reflect(-viewDir, normal);
    
    vec3 F = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);
    
    vec3 Ks = F;
    vec3 Kd = 1.0 - Ks;
    Kd *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 diffuse = irradiance * albedo;

    float roughnessDelta = 1.0 / maxLOD;
    float sampleRoughness = roughness * maxLOD;
    vec3 prefilteredColor1 = textureLod(prefilteredEnvMap, R, sampleRoughness).rgb;
    vec3 prefilteredColor2 = textureLod(prefilteredEnvMap, R, max(sampleRoughness - roughnessDelta, 0.0)).rgb;
    vec3 prefilteredColor = mix(prefilteredColor1, prefilteredColor2, fract(sampleRoughness));
    vec2 brdfUV = vec2(clamp(max(dot(normal, viewDir), 0.0), 0.01, 0.99), roughness);
    vec2 envBRDF = texture(brdfLUT, brdfUV).rg;
    
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    

    vec3 base = (Kd * diffuse + specular) * ambientOcclussion;
    vec3 sheen = calcSheen(normal, viewDir, sheenColor, sheenIntensity) * ambientOcclussion;
    
    return base + sheen;
}
