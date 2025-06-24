#version 450

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0, rgba16f) uniform writeonly image2D brdfLUT;

const uint SAMPLE_COUNT = 1024u;

const float PI = 3.14159265359;

vec3 importanceSampleGGX(vec2 Xi, float roughness, vec3 N)
{
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    
    // Transform H to world space
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float a = roughness;
    float a2 = a * a;
    float ggx1 = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    float ggx2 = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
    return 0.5 / (ggx1 + ggx2 + 1e-5);
}

void main()
{
    ivec2 size = imageSize(brdfLUT);
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);

    if (coord.x >= size.x || coord.y >= size.y)
        return;

    vec2 uv = vec2(coord) / vec2(size);

    float NdotV = uv.x;
    float roughness = uv.y;

    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV); // sin(theta)
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = vec2(float(i) / float(SAMPLE_COUNT), fract(float(i) * 0.61803398875)); // Low-discrepancy sampling (golden ratio)
        vec3 H = importanceSampleGGX(Xi, roughness, N);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = geometrySmith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV + 1e-5);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    imageStore(brdfLUT, coord, vec4(A, B, 0.0, 1.0));
}
