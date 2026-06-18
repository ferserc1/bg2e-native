#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_shading_language_include : require

layout(location = 0) rayPayloadInEXT GIPayload {
    vec3 hitDirectLight;
    vec3 hitAlbedo;
    vec3 hitNormal;
    vec3 hitPosition;
    uint didHit;
} payload;

void main() {
    payload.hitDirectLight = vec3(0.0);
    payload.hitAlbedo = vec3(1.0);
    payload.hitNormal = vec3(0.0);
    payload.hitPosition = vec3(0.0);
    payload.didHit = 0u;
}
