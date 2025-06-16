//
//  uniforms.glsl

struct PBRMaterialData
{
    vec4 albedo;

    vec2 albedoScale;
    vec2 normalScale;
    vec2 metalnessScale;
    vec2 roughnessScale;

    float metalness;
    float roughness;

    int albedoUVSet;
    int normalUVSet;
    int metalnessUVSet;
    int roughnessUVSet;
    int aoUVSet;
};

struct Light
{
    vec3 position;
    float intensity;
    vec4 color;
    vec3 direction;
    int type;
};

const int LIGHT_COUNT = 8;
