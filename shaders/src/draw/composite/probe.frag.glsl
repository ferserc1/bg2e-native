#version 450
#extension GL_GOOGLE_include_directive : require
#include "lib/color.glsl"
layout(location = 0) out vec4 color;
void main()
{
    color = vec4(drawColorCorrection(vec3(0.5), 1.0, 2.2, 0.0, 1.0), 1.0);
}
