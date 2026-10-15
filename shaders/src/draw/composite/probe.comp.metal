#include "lib/color.metal"
kernel void compMain(texture2d<float, access::write> outputImage [[texture(0)]], uint2 position [[thread_position_in_grid]])
{
    if (position.x >= outputImage.get_width() || position.y >= outputImage.get_height())
    {
        return;
    }
    outputImage.write(float4(drawColorCorrection(float3(0.5f), 1.0f, 2.2f, 0.0f, 1.0f), 1.0f), position);
}
