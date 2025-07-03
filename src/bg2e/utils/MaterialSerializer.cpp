//
//  MaterialSerializer.cpp

#include <bg2e/utils/MaterialSerializer.hpp>
#include <json-parser.hpp>

namespace bg2e::utils {

base::Texture * getTexture(const std::filesystem::path& basePath, const std::string& file)
{
    auto fullPath = std::filesystem::path(basePath).append(file);
    auto texture = new base::Texture();
    texture->setImageFilePath(fullPath.string());
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
    
    if (mat["name"])
    {
        result.setName(mat["name"]->stringValue(""));
    }
    if (mat["groupName"])
    {
        result.setGroupName(mat["groupName"]->stringValue(""));
    }
    
    
    // Albedo
    if (mat["diffuse"] && mat["diffuse"]->isVec4())
    {
        result.setAlbedo(mat["diffuse"]->vec4Value());
    }
    else if (mat["diffuse"] && mat["diffuse"]->isVec3())
    {
        result.setAlbedo(mat["diffuse"]->vec3Value());
    }
    else if (mat["diffuse"] && mat["diffuse"]->isString())
    { 
        result.setAlbedo(getTexture(basePath, mat["diffuse"]->stringValue()));
        result.setAlbedoScale(mat["diffuseScale"] ? mat["diffuseScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setAlbedoUVSet(mat["diffuseUV"] ? mat["diffuseUV"]->numberValue(0) : 0);
    }
    
    // Metalness
    if (mat["metallic"] && mat["metallic"]->isNumber())
    {
        result.setMetalness(mat["metallic"]->numberValue(0.0f));
    }
    else if (mat["metallic"] && mat["metallic"]->isString())
    {
        result.setMetalness(getTexture(basePath, mat["metallic"]->stringValue()));
        result.setMetalnessChannel(mat["metallicChannel"] ? mat["metallicChannel"]->numberValue(0) : 0);
        result.setMetalnessScale(mat["metallicScale"] ? mat["metallicScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setMetalnessUVSet(mat["metallicUV"] ? mat["metallicUV"]->numberValue(0) : 0);
    }
    
    // Roughness
    if (mat["roughness"] && mat["roughness"]->isNumber())
    {
        result.setRoughness(mat["roughness"]->numberValue(1.0f));
    }
    else if (mat["roughness"] && mat["roughness"]->isString())
    {
        result.setRoughness(getTexture(basePath, mat["roughness"]->stringValue()));
        result.setRoughnessChannel(mat["roughnessChannel"] ? mat["roughnessChannel"]->numberValue(0) : 0);
        result.setRoughnessScale(mat["roughnessScale"] ? mat["roughnessScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setRoughnessUVSet(mat["roughnessUV"] ? mat["roughnessUV"]->numberValue(0) : 0);
    }
    
    // Normal
    if (mat["normal"] && mat["normal"]->isString())
    {
        result.setNormalTexture(getTexture(basePath, mat["normal"]->stringValue()));
        result.setNormalScale(mat["normalScale"] ? mat["normalScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setNormalUVSet(mat["normalUV"] ? mat["normalUV"]->numberValue(0) : 0);
    }
    
    // Ambient Occlussion
    if (mat["ambientOcclussion"] && mat["ambientOcclussion"]->isString())
    {
        result.setAoTexture(getTexture(basePath, mat["ambientOcclussion"]->stringValue()));
        result.setAoScale(mat["ambientOcclussionScale"] ? mat["ambientOcclussionScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setAoChannel(mat["ambientOcclussionChannel"] ? mat["ambientOcclussionChannel"]->numberValue(0) : 0);
        result.setAoUVSet(mat["ambientOcclussionUV"] ? mat["ambientOcclussionUV"]->numberValue(1) : 1);
    }
    
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
