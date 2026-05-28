#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : enable

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

void main() {
    payload.didHit = 1u;
    payload.hitDistance = gl_HitTEXT;

    // TODO: generate the correct hit color for the payload
    float attenuation = 1.0 - clamp(gl_HitTEXT / 50.0, 0.0, 1.0);
    payload.hitColor = vec3(0.5) * attenuation;
}