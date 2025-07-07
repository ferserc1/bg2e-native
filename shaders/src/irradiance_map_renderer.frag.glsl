#version 450
#extension GL_ARB_shading_language_include : require

#include "lib/color_correction.glsl"

layout (location = 0) in vec3 fragNormal;
layout (location = 1) in flat int inCurrentMipLevel;
layout (location = 2) in flat int inTotalMipLevels;
layout (location = 3) in flat int inFaceIndex;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform samplerCube skyTexture;

void main()
{
    // Compute the irradiance using the pre-filtered environment map
    const float sampleDelta = 0.025;
    const float PI = 3.14159265359;
    vec3 normal = normalize(fragNormal);
    vec3 irradiance = vec3(0.0);
    vec3 maxRange = vec3(40.0);
    
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up = cross(normal,right);

    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // Spherical to cartesian
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

            // tangent space to world space
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            irradiance += clamp(texture(skyTexture, sampleVec).rgb, vec3(0.0), maxRange) * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    outFragColor = vec4(irradiance, 1.0);
}
