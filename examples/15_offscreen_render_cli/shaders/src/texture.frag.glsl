#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;

layout(set = 1, binding = 1) uniform sampler2D colorTex;

void main() {
    vec3 color = texture(colorTex, inTexCoord).rgb;
    outColor = vec4(color, 1.0f);
}
