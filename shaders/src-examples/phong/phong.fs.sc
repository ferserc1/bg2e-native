$input v_normal, v_fragPos, v_tex0

#include <bgfx_shader.sh>

uniform vec4 lightPosition;

void main() {
    vec3 lightDir = normalize(lightPosition.xyz - v_fragPos);
    float diff = max(dot(v_normal,lightDir), 0.0);
    vec3 lightColor = vec3(1.0,1.0,1.0);
    vec3 diffuse = lightColor * diff;
    gl_FragColor = vec4(diffuse,1.0) + vec4(v_tex0.x, v_tex0.y, 0.0, 0.0);
}