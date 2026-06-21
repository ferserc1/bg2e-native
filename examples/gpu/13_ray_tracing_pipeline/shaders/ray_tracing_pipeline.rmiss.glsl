#version 460
#extension GL_EXT_ray_tracing : enable

struct HitInfo {
    vec3  hitPos;
    vec3  normal;
    vec3  albedo;
    vec3  emission;
    float roughness;
    int   hit;
};
layout(location = 0) rayPayloadInEXT HitInfo payload;

void main()
{
    // No surface hit: the path tracer treats this as the (black) background.
    payload.hit = 0;
}
