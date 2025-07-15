//
//  MaterialSerializer.cpp

#include <bg2e/utils/MaterialSerializer.hpp>
#include <bg2e/json/JsonParser.hpp>

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
    json::JsonNode * node,
    const std::filesystem::path& basePath,
    base::MaterialAttributes & result
)
{
    if (!node->isObject())
    {
        return false;
    }
    
    auto mat = node->objectValue();
    
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
    json::JsonParser parser(jsonString);
    auto jsonData = parser.parse();
    
    return parseMaterial(jsonData.get(), basePath, result);
}

bool MaterialSerializer::deserializeMaterialArray(
    const std::string& jsonString,
    const std::filesystem::path& basePath,
    std::vector<base::MaterialAttributes>& result
) {
    json::JsonParser parser(jsonString);
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

std::string MaterialSerializer::serializeMaterial(
    base::MaterialAttributes& mat,
    std::vector<std::shared_ptr<base::Texture>> & uniqueTextures
) {
    using namespace bg2e::json;
    
    auto matJson = JSON(JsonObject{
        { "name", JSON(mat.name()) },
        { "groupName", JSON(mat.groupName()) },
        { "type", JSON("pbr") },
        { "class", JSON("PBRMaterial") }
    });
    auto & obj = matJson->objectValue();
    
    if (mat.albedoTexture().get())
    {
        obj["diffuse"] = JSON(mat.albedoTexture()->imageFilePath());
        addUniqueTexture(mat.albedoTexture(), uniqueTextures);
        obj["diffuseScale"] = JSON(mat.albedoScale());
        obj["diffuseUV"] = JSON(mat.albedoUVSet());
    }
    else {
        obj["diffuse"] = JSON(mat.albedo());
    }
    
    if (mat.metalnessTexture().get())
    {
        obj["metallic"] = JSON(mat.metalnessTexture()->imageFilePath());
        addUniqueTexture(mat.metalnessTexture(), uniqueTextures);
        obj["metallicChannel"] = JSON(mat.metalnessChannel());
        obj["metallicScale"] = JSON(mat.metalnessScale());
        obj["metallicUV"] = JSON(mat.metalnessUVSet());
    }
    else {
        obj["metallic"] = JSON(mat.metalness());
    }
    
    if (mat.roughnessTexture().get())
    {
        obj["roughness"] = JSON(mat.roughnessTexture()->imageFilePath());
        addUniqueTexture(mat.roughnessTexture(), uniqueTextures);
        obj["roughnessChannel"] = JSON(mat.roughnessChannel());
        obj["roughnessScale"] = JSON(mat.roughnessScale());
        obj["roughnessUV"] = JSON(mat.roughnessUVSet());
    }
    else {
        obj["roughness"] = JSON(mat.roughness());
    }
    
    if (mat.normalTexture().get())
    {
        obj["normal"] = JSON(mat.normalTexture()->imageFilePath());
        addUniqueTexture(mat.normalTexture(), uniqueTextures);
        obj["normalScale"] = JSON(mat.normalScale());
        obj["normalUV"] = JSON(mat.normalUVSet());
    }
    
    if (mat.aoTexture().get())
    {
        obj["ambientOcclussion"] = JSON(mat.aoTexture()->imageFilePath());
        addUniqueTexture(mat.aoTexture(), uniqueTextures);
        obj["ambientOcclussionScale"] = JSON(mat.aoScale());
        obj["ambientOcclussionChannel"] = JSON(mat.aoChannel());
        obj["ambientOcclussionUV"] = JSON(mat.aoUVSet());
    }
    
    return matJson->serialize();
}

std::string MaterialSerializer::serializeMaterialArray(
    std::vector<base::MaterialAttributes>& mat,
    std::vector<std::shared_ptr<base::Texture>> & uniqueTextures
) {
    std::string result = "[";
    std::string sep = "";
    for (auto & m : mat)
    {
        result += sep + serializeMaterial(m, uniqueTextures);
        sep = ",";
    }
    return result + "]";
}

void MaterialSerializer::addUniqueTexture(
    std::shared_ptr<base::Texture> tex,
    std::vector<std::shared_ptr<base::Texture>>& textures
) {
    // Only add textures associated with a file
    if (tex->imageFilePath() == "")
    {
        return;
    }
    
    for (auto & t : textures)
    {
        if (t->imageFilePath() == tex->imageFilePath())
        {
            return;
        }
    }
    
    textures.push_back(tex);
}

}
