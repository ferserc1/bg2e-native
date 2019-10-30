$input v_normal, v_fragPos, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 lightPosition;

SAMPLER2D(s_diffuseTexture, 0);

void main() {
    vec3 textureColor = texture2D(s_diffuseTexture, v_texcoord0).rgb;
    vec3 lightDir = normalize(lightPosition.xyz - v_fragPos);
    float diff = max(dot(v_normal,lightDir), 0.0);
    vec3 lightColor = vec3(1.0,1.0,1.0);
    vec3 diffuse = lightColor * diff;
    
    diffuse = diffuse * textureColor;
    gl_FragColor = vec4(diffuse,1.0);
}