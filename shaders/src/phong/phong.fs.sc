$input v_normal, v_fragPos, v_texcoord0, v_texcoord1, v_tangent, v_bitangent

#include <bgfx_shader.sh>

uniform vec3 u_lightDirection;

SAMPLER2D(u_diffuse, 0);
SAMPLER2D(u_normal, 1);
SAMPLER2D(u_ambientOcclussion, 2);
SAMPLER2D(u_roughness, 3);

uniform vec4 u_fresnel;

vec3 combineNormalWithMap(vec3 normalCoord, vec3 tangent, vec3 bitangent, vec3 normalMapValue) {
    mat3 tbnMat = mat3( tangent.x, bitangent.x, normalCoord.x,
                        tangent.y, bitangent.y, normalCoord.y,
                        tangent.z, bitangent.z, normalCoord.z);
    return normalize(mul(tbnMat, normalMapValue));
}

void main() {
    vec3 textureColor = texture2D(u_diffuse, v_texcoord0).rgb;
    vec3 normalColor = texture2D(u_normal, v_texcoord0).rgb * 2.0 - 1.0;
    vec3 aoColor = texture2D(u_ambientOcclussion, v_texcoord0).rgb;
    float roughness = texture2D(u_roughness, v_texcoord0).x;

    vec3 combinedNormal = combineNormalWithMap(v_normal, v_tangent, v_bitangent, normalColor);
    //vec3 lightDir = normalize(lightPosition.xyz - v_fragPos);
    float diff = max(dot(combinedNormal,-u_lightDirection), 0.0);
    vec3 lightColor = vec3(1.0,1.0,1.0);
    vec3 diffuse = lightColor * diff;

    float shininess = (1.0 / roughness) * 255.0f;
    // TODO: specular with shininess and u_fresnel;
    
    diffuse = diffuse * textureColor * aoColor;
    gl_FragColor = vec4(diffuse,1.0);
}