#include "lib/color.metal"
fragment float4 fragMain()
{
    return float4(drawColorCorrection(float3(0.5f), 1.0f, 2.2f, 0.0f, 1.0f), 1.0f);
}
