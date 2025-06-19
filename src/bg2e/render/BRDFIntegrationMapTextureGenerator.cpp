//
//  BRDFIntegrationMapTextureGenerator.cpp
#include <bg2e/render/BRDFIntegrationMapTextureGenerator.hpp>
#include <bg2e/math/all.hpp>
#include <bg2e/base/PlatformTools.hpp>

#include <numbers>

#include <stb_image/stb_image.h>

namespace bg2e::render {

float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

glm::vec2 hammersley(uint32_t i, uint32_t N)
{
    return glm::vec2(static_cast<float>(i) / static_cast<float>(N), radicalInverse_VdC(i));
}

glm::vec3 importanceSampleGGX(const glm::vec2& Xi, const glm::vec3& N, float roughness)
{
    using namespace glm;
    
    float a = roughness * roughness;
	
    float phi = 2.0 * std::numbers::pi * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}


float geometrySmith(const glm::vec3& normal, const glm::vec3& viewDir, const glm::vec3& lightDir, float roughness)
{
    using namespace glm;
    float NdotV = dot(normal, viewDir);
    NdotV = NdotV > 0.0f ? NdotV : 0.0f;
    float NdotL = dot(normal, lightDir);
    NdotL = NdotL > 0.0f ? NdotL : 0.0f;
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

glm::vec2 integrateBRDF(float NdotV, float roughness)
{
    using namespace glm;
    vec3 V;
    V.x = std::sqrt(1.0f - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;
    
    float A = 0.0f;
    float B = 0.0f;
    
    vec3 N = vec3(0.0f, 0.0f, 1.0);
    const uint32_t SAMPLE_COUNT = 512;
    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0f * dot(V, H) * H - V);
        
        float NdotL = L.z > 0.0f ? L.z : 0.0f;
        float NdotH = H.z > 0.0f ? H.z : 0.0f;
        float VdotH = dot(V, H);
        VdotH = VdotH > 0.0f ? VdotH : 0.0f;
        
        if (NdotL > 0.0f)
        {
            float G = geometrySmith(N, V, L, roughness);
            float G_vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            
            A += (1.0f - Fc) * G_vis;
            B += Fc * G_vis;
        }
        
    }
    A /= static_cast<float>(SAMPLE_COUNT);
    B /= static_cast<float>(SAMPLE_COUNT);
    return vec2(A, B);
}

uint8_t* BRDFIntegrationMapTextureGenerator::generate()
{
//    using namespace glm;
//    auto size = _width * _height * _bpp;
//    uint8_t * buffer = new uint8_t[size];
//    
//    uint32_t lineWidth = _width * _bpp;
//    for (uint32_t j = 0; j < _height; ++j)
//    {
//        for (uint32_t i = 0; i < _width; ++i)
//        {
//            float u = static_cast<float>(i) / static_cast<float>(_width);
//            float v = static_cast<float>(_height - j) / static_cast<float>(_height);
//            vec2 result = integrateBRDF(u, v);
//            buffer[j * lineWidth + i * _bpp] = static_cast<uint8_t>(result.x * 255.0f);
//            buffer[j * lineWidth + i * _bpp + 1] = static_cast<uint8_t>(result.y * 255.0f);
//            buffer[j * lineWidth + i * _bpp + 2] = 0;
//            if (_bpp == 4) {
//                buffer[j * lineWidth + i * _bpp + 3] = 255;
//            }
//        }
//    }
//    
//    return buffer;
    auto assetPath = bg2e::base::PlatformTools::assetPath();
    assetPath.append("ibl_brdf_lut.png");

    int width, height, channels;
    unsigned char* data = stbi_load(assetPath.string().c_str(), &width, &height, &channels, 4);
    
    setWidth(width);
    setHeight(height);
    setBytesPerPixel(channels);
    
    return data;
}
    
std::string BRDFIntegrationMapTextureGenerator::imageIdentifier()
{
    return "BRDFIntegrationMapTexture";
}
    
}
