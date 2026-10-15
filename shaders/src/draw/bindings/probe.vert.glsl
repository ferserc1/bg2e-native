#version 450
#extension GL_GOOGLE_include_directive : require
#define DRAW_BINDINGS_VERTEX
#include "../lib/bindings.glsl"
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv0;
layout(location = 3) in vec2 uv1;
layout(location = 4) in vec3 tangent;
layout(location = 0) out vec2 fragUV0;
layout(location = 1) out vec2 fragUV1;
void main()
{
    fragUV0 = uv0;
    fragUV1 = uv1;
    gl_Position = drawFrame.data.projection * drawFrame.data.view * drawObject.data.model * vec4(position, 1.0);
}
