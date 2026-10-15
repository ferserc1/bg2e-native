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
