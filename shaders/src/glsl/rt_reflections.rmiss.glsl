#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_shading_language_include : require

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

void main() {
    payload.hitColor = vec3(0.0);
    payload.hitDistance = 0.0;
    payload.didHit = 0u;
}