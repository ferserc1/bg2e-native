#ifndef _BG2_SCENE_BG2MATERIAL_HPP_
#define _BG2_SCENE_BG2MATERIAL_HPP_

#include <json.hpp>

#include <array>
#include <string>

namespace bg2scene {

class Bg2Material {
public:
    Bg2Material();
    Bg2Material(json::JsonObject&);
    
    std::string name = "";
    std::string matClass = "PBRMaterial";
    std::string groupName = "";
    std::string diffuseTexture = "";
    std::array<float, 4> diffuseColor = { 1.f, 1.f, 1.f, 1.f };
    std::string metallicTexture = "";
    float metallic = 0.f;
    std::string roughnessTexture = "";
    float roughness = 1.f;
    std::string normalTexture = "";
    std::string ambientOcclussionTexture = "";
    uint32_t metallicChannel = 0;
    uint32_t roughnessChannel = 0;
    uint32_t lightEmissionChannel = 0;
    uint32_t heightChannel = 0;
    uint32_t diffuseUV = 0;
    uint32_t metallicUV = 0;
    uint32_t roughnessUV = 0;
    uint32_t fresnelUV = 0;
    uint32_t ambientOcclussionUV = 1;
    uint32_t lightEmissionUV = 0;
    uint32_t heightUV = 0;
    uint32_t normalUV = 0;
    float alphaCutoff = 0.5f;
    bool isTransparent = false;
    bool castShadows = true;
    bool cullFace = true;
    bool unlit = false;
    bool visible = true;
    bool visibleToShadows = true;
    std::array<float, 2> diffuseScale = { 1.f, 1.f };
    std::array<float, 2> metallicScale = { 1.f, 1.f };
    std::array<float, 2> roughnessScale = { 1.f, 1.f };
    std::array<float, 2> fresnelScale = { 1.f, 1.f };
    std::array<float, 2> lightEmissionScale = { 1.f, 1.f };
    std::array<float, 2> heightScale = { 1.f, 1.f };
    std::array<float, 2> normalScale = { 1.f, 1.f };
    std::array<float, 2> diffuseOffset = { 0.f, 0.f };
    std::array<float, 2> metallicOffset = { 0.f, 0.f };
    std::array<float, 2> roughnessOffset = { 0.f, 0.f };
    std::array<float, 2> lightEmissionOffset = { 0.f, 0.f };
    std::array<float, 2> heightOffset = { 0.f, 0.f };
    std::array<float, 2> normalOffset = { 0.f, 0.f };
    
};

}

#endif
