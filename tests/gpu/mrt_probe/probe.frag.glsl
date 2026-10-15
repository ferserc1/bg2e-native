#version 450
layout(push_constant) uniform FragmentParameters {
    layout(offset = 4) float multiplier;
} parameters;
layout(location = 0) out vec4 color0;
layout(location = 1) out vec4 color1;
layout(location = 2) out vec4 color2;
layout(location = 3) out vec4 color3;
layout(location = 4) out vec4 color4;
layout(location = 5) out vec4 color5;
void main()
{
    float m = parameters.multiplier;
    color0 = vec4(m, 0, 0, 1);
    color1 = vec4(0, m, 0, 1);
    color2 = vec4(0, 0, m, 1);
    color3 = vec4(m, m, 0, 1);
    color4 = vec4(m, 0, m, 1);
    color5 = vec4(0, m, m, 1);
}
