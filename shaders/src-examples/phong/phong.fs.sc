$input v_normal, v_fragPos, v_texcoord0, v_texcoord1, v_tangent, v_bitangent

#include <bgfx_shader.sh>

uniform vec4 lightPosition;

SAMPLER2D(s_diffuseTexture, 0);
SAMPLER2D(s_normalTexture, 1);

vec3 combineNormalWithMap(vec3 normalCoord, vec3 tangent, vec3 bitangent, vec3 normalMapValue) {
    mat3 tbnMat = mat3( tangent.x, bitangent.x, normalCoord.x,
                        tangent.y, bitangent.y, normalCoord.y,
                        tangent.z, bitangent.z, normalCoord.z);
    return normalize(mul(tbnMat, normalMapValue));
}

void main() {
    vec3 textureColor = texture2D(s_diffuseTexture, v_texcoord0).rgb;
    vec3 normalColor = texture2D(s_normalTexture, v_texcoord0).rgb * 2.0 - 1.0;
    vec3 combinedNormal = combineNormalWithMap(v_normal, v_tangent, v_bitangent, normalColor);
    vec3 lightDir = normalize(lightPosition.xyz - v_fragPos);
    float diff = max(dot(combinedNormal,lightDir), 0.0);
    vec3 lightColor = vec3(1.0,1.0,1.0);
    vec3 diffuse = lightColor * diff;
    
    diffuse = diffuse * textureColor;
    gl_FragColor = vec4(diffuse,1.0);
}