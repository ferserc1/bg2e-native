#version 460
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_ray_tracing : require

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT ReflectionPayload {
    vec3 hitColor;
    float hitDistance;
    uint didHit;
} payload;

void main() {
    payload.didHit = 1u;
    payload.hitDistance = gl_HitTEXT;

    vec3 bary = vec3(
        1.0 - attribs.x - attribs.y,
        attribs.x,
        attribs.y
    );

    float edge = min(min(bary.x, bary.y), bary.z);

    float line = smoothstep(0.0, 0.02, edge);

    payload.hitColor = mix(vec3(1.0), bary, line);
    payload.hitDistance = gl_HitTEXT;
    payload.didHit = 1;
}