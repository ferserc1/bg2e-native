#version 450
#extension GL_ARB_shading_language_include : require

// 20% GPU, 60fps

#include "lib/color_correction.glsl"
#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"
#include "lib/constants.glsl"

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

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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


void main()
{
    vec3 albedo = SRGB2Lineal(texture(albedoTex, inUV0), pushConstant.gamma).rgb * objectData.material.albedo.rgb;
    float metallic = objectData.material.metalness;
    const float minRoughness = 0.05; // Minimum roughness value: even a mirror have some roughness
    float roughness = max(objectData.material.roughness, minRoughness);
    float ao = 1.0; // ambient occlusion, can be added later
    mat3 TBN = inTBN;
    
    vec3 N = normalize(TBN * sampleNormal(normalTex, inUV0));
    vec3 V = normalize(inViewPos - inFragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < inLightCount; ++i)
    {
        Light light = inLights[i];
        
        // calculate per-light radiance
        vec3 L = normalize(light.position - inFragPos);
        vec3 H = normalize(V + L);
        float distance    = length(light.position - inFragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = light.color.rgb * light.intensity * attenuation;
        
        // cook-torrance brdf
        float NDF = distributionGGX(N, H, roughness);        
        float G   = geometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    outColor = vec4(color, 1.0);
} 
