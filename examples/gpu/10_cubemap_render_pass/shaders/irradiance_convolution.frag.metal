#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 dir;
};

constant float PI = 3.14159265359;

fragment float4 fragMain(VertexOut in [[stage_in]],
                         texturecube<float> uEnv     [[texture(0)]],
                         sampler            uSampler [[sampler(0)]])
{
    float3 N = normalize(in.dir);

    float3 up    = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float3 irradiance  = float3(0.0);
    float  sampleDelta = 0.05;
    float  nrSamples   = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            float3 tangentSample = float3(sin(theta) * cos(phi),
                                          sin(theta) * sin(phi),
                                          cos(theta));
            float3 sampleVec = tangentSample.x * right
                             + tangentSample.y * up
                             + tangentSample.z * N;

            irradiance += uEnv.sample(uSampler, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples += 1.0;
        }
    }

    irradiance = PI * irradiance / nrSamples;
    return float4(irradiance, 1.0);
}
