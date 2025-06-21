#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"
#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"
#include "lib/pbr.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec2 inUV1;
layout(location = 3) in vec3 inViewPos;
layout(location = 4) in vec3 inFragPos;
layout(location = 5) in flat int inLightCount;
layout(location = 6) in mat3 inTBN;
layout(location = 9) in flat Light[LIGHT_COUNT] inLights;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 1, binding = 3) uniform sampler2D metallicTex;
layout(set = 1, binding = 4) uniform sampler2D roughnessTex;
layout(set = 1, binding = 5) uniform sampler2D aoTex;

layout(push_constant) uniform PushConstant
{
    float gamma;
} pushConstant;

layout(set = 2, binding = 0) uniform samplerCube irradianceMap;
layout(set = 2, binding = 1) uniform samplerCube prefilteredEnvMap;
layout(set = 2, binding = 2) uniform sampler2D brdfLUT;
layout(set = 2, binding = 3) uniform EnvironmentData {
    float maxReflectionLOD;
} environmentData;

void main()
{
    // TODO: get texture from the appropiate UV map
    // TODO: add the texture scale
    vec3 albedo = SRGB2Lineal(texture(albedoTex, inUV0), pushConstant.gamma).rgb * objectData.material.albedo.rgb;
    float metallic = texture(metallicTex, inUV0).r * objectData.material.metalness;
    const float minRoughness = 0.05; // Minimum roughness value: even a mirror have some roughness
    float roughness = texture(roughnessTex, inUV0).r * objectData.material.roughness;
    roughness = max(roughness, minRoughness);
    float ao = texture(aoTex, inUV0).r;
    mat3 TBN = inTBN;
    
    vec3 normal = normalize(TBN * sampleNormal(normalTex, inUV0));
    vec3 viewDir = normalize(inViewPos - inFragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < inLightCount; ++i)
    {
        Lo += calcRadiance(inLights[i], viewDir, inFragPos, metallic, roughness, F0, normal, albedo);
    }
  
    // ambient light
    // vec3 R = reflect(-viewDir, normal);
    
    // vec3 F = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);

    // vec3 Ks = F;
    // vec3 Kd = 1.0 - Ks;
    // Kd *= 1.0 - metallic;

    // vec3 irradiance = texture(irradianceMap, normal).rgb;
    // vec3 diffuse = irradiance * albedo;

    // const float MAX_REFLECTION_LOD = 10.0; // TODO: Set this as uniform
    // vec3 prefilteredColor = textureLod(prefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    // vec2 envBRDF = texture(brdfLUT, vec2(max(dot(normal, viewDir), 0.0), roughness)).rg;
    // vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    // vec3 ambient = (Kd * diffuse + specular) * ao;

    //const float MAX_REFLECTION_LOD = 10.0; // TODO: Set this as uniform
    vec3 ambient = calcAmbientLight(
        viewDir, normal, F0, 
        albedo, metallic, roughness,
        irradianceMap, prefilteredEnvMap,
        environmentData.maxReflectionLOD,
        brdfLUT, ao);

    vec3 color = ambient + Lo;

    outColor = lineal2SRGB(vec4(color, 1.0), pushConstant.gamma);
}
