//
//  MaterialSerializer.cpp

#include <bg2e/utils/MaterialSerializer.hpp>
#include <json-parser.hpp>

namespace bg2e::utils {

base::Texture * getTexture(const std::filesystem::path& basePath, const std::string& file)
{
    auto fullPath = std::filesystem::path(basePath).append(file);
    auto texture = new base::Texture();
    texture->setImageFilePath(fullPath);
    texture->setMagFilter(base::Texture::FilterLinear);
    texture->setMinFilter(base::Texture::FilterLinear);
    texture->setUseMipmaps(true);
    return texture;
}

bool parseMaterial(
    bg2scene::json::JsonNode * node,
    const std::filesystem::path& basePath,
    base::MaterialAttributes & result
)
{
    if (!node->isObject())
    {
        return false;
    }
    
    auto mat = node->objectValue();
    
    // TODO: Check all access to json properties because can be null
    // Create an utility function to access json object properties that
    // returns a null Json node if the property does not exist
    
    result.setName(mat["name"]->stringValue(""));
    result.setGroupName(mat["groupName"]->stringValue(""));
    
    // Albedo
    if (mat["diffuse"]->isVec4())
    {
        result.setAlbedo(mat["diffuse"]->vec4Value());
    }
    else if (mat["diffuse"]->isVec3())
    {
        result.setAlbedo(mat["diffuse"]->vec3Value());
    }
    else if (mat["diffuse"]->isString())
    { 
        result.setAlbedo(getTexture(basePath, mat["diffuse"]->stringValue()));
        result.setAlbedoScale(mat["diffuseScale"]->vec2Value({ 1, 1 }));
        result.setAlbedoUVSet(mat["diffuseUV"]->numberValue(0));
    }
    
    // Metalness
    if (mat["metallic"]->isNumber())
    {
        result.setMetalness(mat["metallic"]->numberValue(0.0f));
    }
    else if (mat["metallic"]->isString())
    {
        result.setMetalness(getTexture(basePath, mat["metallic"]->stringValue()));
        result.setMetalnessChannel(mat["metallicChannel"]->numberValue(0));
        result.setMetalnessScale(mat["metallicScale"]->vec2Value({ 1, 1 }));
        result.setMetalnessUVSet(mat["metallicUV"]->numberValue(0));
    }
    
    // Roughness
    if (mat["roughness"]->isNumber())
    {
        result.setRoughness(mat["roughness"]->numberValue(1.0f));
    }
    else if (mat["roughness"]->isString())
    {
        result.setRoughness(getTexture(basePath, mat["roughness"]->stringValue()));
        result.setRoughnessChannel(mat["roughnessChannel"]->numberValue(0));
        result.setRoughnessScale(mat["roughnessScale"]->vec2Value({ 1, 1 }));
        result.setRoughnessUVSet(mat["roughnessUV"]->numberValue(0));
    }
    
    // Normal
    if (mat["normal"]->isString())
    {
        result.setNormalTexture(getTexture(basePath, mat["normal"]->stringValue()));
        result.setNormalScale(mat["normalScale"]->vec2Value({ 1, 1 }));
        result.setNormalUVSet(mat["normalUV"]->numberValue(0));
    }
    
    // Ambient Occlussion
//    if (mat["ambientOcclussion"]->isString())
//    {
//        result.setAoTexture(getTexture(basePath, mat["ambientOcclussion"]->stringValue()));
//        result.setAoScale(mat["ambientOcclussionScale"]->vec2Value({ 1, 1 }));
//        result.setAoChannel(mat["ambientOcclussionChannel"]->numberValue(0));
//        result.setAoUVSet(mat["ambientOcclussionUV"]->numberValue(0));
//    }
    
    return true;
}

bool MaterialSerializer::deserializeMaterial(
    const std::string& jsonString,
    const std::filesystem::path& basePath,
    base::MaterialAttributes& result
) {
    bg2scene::json::JsonParser parser(jsonString);
    auto jsonData = parser.parse();
    
    return parseMaterial(jsonData.get(), basePath, result);
}

bool MaterialSerializer::deserializeMaterialArray(
    const std::string& jsonString,
    const std::filesystem::path& basePath,
    std::vector<base::MaterialAttributes>& result
) {
    bg2scene::json::JsonParser parser(jsonString);
    auto jsonData = parser.parse();
    if (!jsonData->isList())
    {
        return false;
    }
    
    for (auto matItem : jsonData->listValue())
    {
        base::MaterialAttributes mat;
        if (!parseMaterial(matItem.get(), basePath, mat))
        {
            return false;
        }
        result.push_back(mat);
    }
    return true;
}

}
