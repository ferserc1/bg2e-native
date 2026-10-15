#version 450
#extension GL_GOOGLE_include_directive : require
#include "lib/pbr.glsl"
layout(location = 0) out vec4 color;
void main()
{
    color = vec4(drawDirectBRDF(vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0.5), 0.0, 0.5), 1.0);
}
