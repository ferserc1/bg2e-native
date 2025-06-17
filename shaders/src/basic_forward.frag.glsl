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

layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 2, binding = 0) uniform samplerCube giTex;

layout(push_constant) uniform PushConstant
{
    float gamma;
} pushConstant;

layout (set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
} objectData;

void main()
{
    vec3 albedo = SRGB2Lineal(texture(albedoTex, inUV0), pushConstant.gamma).rgb * objectData.material.albedo.rgb;
    float metallic = objectData.material.metalness;
    const float minRoughness = 0.05; // Minimum roughness value: even a mirror have some roughness
    float roughness = max(objectData.material.roughness, minRoughness);
    float ao = 1.0; // ambient occlusion, can be added later
    mat3 TBN = inTBN;
    
    vec3 normal = normalize(TBN * sampleNormal(normalTex, inUV0));
    vec3 viewDir = normalize(inViewPos - inFragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < inLightCount; ++i)
    {
        Light light = inLights[i];
        Lo += calcRadiance(inLights[i], viewDir, inFragPos, metallic, roughness, F0, normal, albedo);
    }
  
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    outColor = lineal2SRGB(vec4(color, 1.0), 2.2);
}
