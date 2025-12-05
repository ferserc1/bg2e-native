
#include <bg2-material.hpp>

namespace bg2scene {

Bg2Material::Bg2Material()
{
    
}

Bg2Material::Bg2Material(json::JsonObject& matData)
{
    name = matData["name"] ? matData["name"]->stringValue(name) : name;
    matClass = matData["class"] ? matData["class"]->stringValue(matClass) : matClass;
    groupName = matData["groupName"] ? matData["groupName"]->stringValue(groupName) : groupName;
    
    if (matData["diffuse"] && matData["diffuse"]->isString()) {
        diffuseTexture = matData["diffuse"]->stringValue();
        diffuseColor = {1.f, 1.f, 1.f, 1.f};
    }
    else if (matData["diffuse"] && matData["diffuse"]->isVec4()) {
        diffuseTexture = "";
        diffuseColor = matData["diffuse"]->vec4Value();
    }
    
    if (matData["metallic"] && matData["metallic"]->isString()) {
        metallicTexture = matData["metallic"]->stringValue();
        metallic = 1.0f;
    }
    else if (matData["metallic"]) {
        metallicTexture = "";
        metallic = matData["metallic"]->numberValue(metallic);
    }
    
    if (matData["roughness"] && matData["roughness"]->isString()) {
        roughnessTexture = matData["roughness"]->stringValue();
        roughness = 1.0f;
    }
    else if (matData["roughness"]) {
        roughnessTexture = "";
        roughness = matData["roughness"]->numberValue(roughness);
    }
    
    normalTexture = matData["normal"] ? matData["normal"]->stringValue(""): normalTexture;
    ambientOcclussionTexture = matData["ambientOcclussion"] ? matData["ambientOcclussion"]->stringValue("") : ambientOcclussionTexture;
    

    metallicChannel = matData["metallicChannel"] ? matData["metallicChannel"]->numberValue(metallicChannel) : metallicChannel;
    roughnessChannel = matData["roughnessChannel"] ? matData["roughnessChannel"]->numberValue(roughnessChannel) : roughnessChannel;
    lightEmissionChannel = matData["lightEmissionChannel"] ? matData["lightEmissionChannel"]->numberValue(lightEmissionChannel) : lightEmissionChannel;
    heightChannel = matData["heightChannel"] ? matData["heightChannel"]->numberValue(heightChannel) : heightChannel;
    diffuseUV = matData["diffuseUV"] ? matData["diffuseUV"]->numberValue(diffuseUV) : diffuseUV;
    metallicUV = matData["metallicUV"] ? matData["metallicUV"]->numberValue(metallicUV) : metallicUV;
    roughnessUV = matData["roughnessUV"] ? matData["roughnessUV"]->numberValue(roughnessUV) : roughnessUV;
    fresnelUV = matData["fresnelUV"] ? matData["fresnelUV"]->numberValue(fresnelUV) : fresnelUV;
    ambientOcclussionUV = matData["ambientOcclussionUV"] ? matData["ambientOcclussionUV"]->numberValue(ambientOcclussionUV) : ambientOcclussionUV;
    lightEmissionUV = matData["lightEmissionUV"] ? matData["lightEmissionUV"]->numberValue(lightEmissionUV) : lightEmissionUV;
    heightUV = matData["heightUV"] ? matData["heightUV"]->numberValue(heightUV) : heightUV;
    normalUV = matData["normalUV"] ? matData["normalUV"]->numberValue(normalUV) : normalUV;
    alphaCutoff = matData["alphaCutoff"] ? matData["alphaCutoff"]->numberValue(alphaCutoff) : alphaCutoff;
    isTransparent = matData["isTransparent"] ? matData["isTransparent"]->boolValue(isTransparent) : isTransparent;
    castShadows = matData["castShadows"] ? matData["castShadows"]->boolValue(castShadows) : castShadows;
    cullFace = matData["cullFace"] ? matData["cullFace"]->boolValue(cullFace) : cullFace;
    unlit = matData["unlit"] ? matData["unlit"]->boolValue(unlit) : unlit;
    visible = matData["visible"] ? matData["visible"]->boolValue(visible) : visible;
    visibleToShadows = matData["visibleToShadows"] ? matData["visibleToShadows"]->boolValue(visibleToShadows) : visibleToShadows;
    diffuseScale = matData["diffuseScale"] ? matData["diffuseScale"]->vec2Value(diffuseScale) : diffuseScale;
    metallicScale = matData["metallicScale"] ? matData["metallicScale"]->vec2Value(metallicScale) : metallicScale;
    roughnessScale = matData["roughnessScale"] ? matData["roughnessScale"]->vec2Value(roughnessScale) : roughnessScale;
    fresnelScale = matData["fresnelScale"] ? matData["fresnelScale"]->vec2Value(fresnelScale) : fresnelScale;
    lightEmissionScale = matData["lightEmissionScale"] ? matData["lightEmissionScale"]->vec2Value(lightEmissionScale) : lightEmissionScale;
    heightScale = matData["heightScale"] ? matData["heightScale"]->vec2Value(heightScale) : heightScale;
    normalScale = matData["normalScale"] ? matData["normalScale"]->vec2Value(normalScale) : normalScale;
    diffuseOffset = matData["diffuseOffset"] ? matData["diffuseOffset"]->vec2Value(diffuseOffset) : diffuseOffset;
    metallicOffset = matData["metallicOffset"] ? matData["metallicOffset"]->vec2Value(metallicOffset) : metallicOffset;
    roughnessOffset = matData["roughnessOffset"] ? matData["roughnessOffset"]->vec2Value(roughnessOffset) : roughnessOffset;
    lightEmissionOffset = matData["lightEmissionOffset"] ? matData["lightEmissionOffset"]->vec2Value(lightEmissionOffset) : lightEmissionOffset;
    heightOffset = matData["heightOffset"] ? matData["heightOffset"]->vec2Value(heightOffset) : heightOffset;
    normalOffset = matData["normalOffset"] ? matData["normalOffset"]->vec2Value(normalOffset) : normalOffset;
}

}
