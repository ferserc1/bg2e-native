#include <metal_stdlib>
using namespace metal;
#include "../lib/bindings.metal"
struct VertexOut {
    float4 position [[position]];
    float2 uv0;
    float2 uv1;
};
fragment float4 fragMain(VertexOut in [[stage_in]], DRAW_FRAGMENT_DATA_ARGUMENTS,
    DRAW_MATERIAL_TEXTURE_ARGUMENTS, DRAW_ENVIRONMENT_TEXTURE_ARGUMENTS)
{
    float3 sum = drawSampleMaterialLod(drawAlbedo, drawAlbedoSampler, drawObject.material, 0u, in.uv0, in.uv1, 0.0).rgb;
    sum += drawSampleMaterialLod(drawMetalness, drawMetalnessSampler, drawObject.material, 1u, in.uv0, in.uv1, 0.0).rgb;
    sum += drawSampleMaterialLod(drawRoughness, drawRoughnessSampler, drawObject.material, 2u, in.uv0, in.uv1, 0.0).rgb;
    sum += drawSampleMaterialLod(drawNormal, drawNormalSampler, drawObject.material, 3u, in.uv0, in.uv1, 0.0).rgb;
    sum += drawSampleMaterialLod(drawAO, drawAOSampler, drawObject.material, 4u, in.uv0, in.uv1, 0.0).rgb;
    sum += drawSampleMaterialLod(drawEmission, drawEmissionSampler, drawObject.material, 5u, in.uv0, in.uv1, 0.0).rgb;
    sum += drawSky.sample(drawSkySampler, float3(1, 0, 0), level(0.0)).rgb;
    sum += drawIrradiance.sample(drawIrradianceSampler, float3(0, 1, 0), level(0.0)).rgb;
    sum += drawPrefilter.sample(drawPrefilterSampler, float3(0, 0, 1), level(drawEnvironment.parameters.x)).rgb;
    sum += drawBRDF.sample(drawBRDFSampler, float2(0.5), level(0.0)).rgb;
    float3 light = drawLights.count.x == 0u ? float3(0.0) : drawLights.lights[0].color.rgb;
    sum += drawObject.material.albedo.rgb + light + drawFrame.cameraPosition.xyz * 0.01;
    return float4(sum / 12.0, 1.0);
}
