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
    // TODO: Parse V2.0 textures:
    // dataType
    // wrapModeX
    // wrapModeY
    // magFilter
    // minFilter
    // target > deprecated
    // size
    // fileName
    // proceduralFunction
    // proceduralParameters
    // name
    // componentFormat
    // See Texture.js file in bg2e-js
    
    
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
    
    if (mat["isTransparent"])
    {
        result.setIsTransparent(mat["isTransparent"]->boolValue(false));
    }
    if (mat["isSolid"])
    {
        result.setIsSolid(mat["isSolid"]->boolValue(false));
    }
    if (mat["visible"])
    {
        result.setVisible(mat["visible"]->boolValue(true));
    }
    
    // Albedo
    if (mat["albedo"] && mat["albedo"]->isVec4())
    {
        result.setAlbedo(mat["albedo"]->vec4Value());
    }
    else if (mat["albedo"] && mat["albedo"]->isVec3())
    {
        result.setAlbedo(mat["albedo"]->vec3Value());
    }
    
    if (mat["albedoTexture"] && mat["albedoTexture"]->isString())
    {
        result.setAlbedoTexture(getTexture(basePath, mat["albedoTexture"]->stringValue()));
        result.setAlbedoScale(mat["albedoScale"] ? mat["albedoScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setAlbedoUVSet(mat["albedoUV"] ? mat["albedoUV"]->numberValue(0) : 0);
    }
    
    // Metalness
    if (mat["metalness"] && mat["metalness"]->isNumber())
    {
        result.setMetalness(mat["metalness"]->numberValue(0.0f));
    }
    
    if (mat["metalnessTexture"] && mat["metalnessTexture"]->isString())
    {
        result.setMetalnessTexture(getTexture(basePath, mat["metallic"]->stringValue()));
        result.setMetalnessChannel(mat["metallicChannel"] ? mat["metallicChannel"]->numberValue(0) : 0);
        result.setMetalnessScale(mat["metallicScale"] ? mat["metallicScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setMetalnessUVSet(mat["metallicUV"] ? mat["metallicUV"]->numberValue(0) : 0);
    }
    
    // Roughness
    if (mat["roughness"] && mat["roughness"]->isNumber())
    {
        result.setRoughness(mat["roughness"]->numberValue(1.0f));
    }
    
    if (mat["roughnessTexture"] && mat["roughnessTexture"]->isString())
    {
        result.setRoughnessTexture(getTexture(basePath, mat["roughnessTexture"]->stringValue()));
        result.setRoughnessChannel(mat["roughnessChannel"] ? mat["roughnessChannel"]->numberValue(0) : 0);
        result.setRoughnessScale(mat["roughnessScale"] ? mat["roughnessScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setRoughnessUVSet(mat["roughnessUV"] ? mat["roughnessUV"]->numberValue(0) : 0);
    }
    
    // Normal
    if (mat["normalTexture"] && mat["normalTexture"]->isString())
    {
        result.setNormalTexture(getTexture(basePath, mat["normalTexture"]->stringValue()));
        result.setNormalScale(mat["normalScale"] ? mat["normalScale"]->vec2Value({ 1, 1 }) : std::array<float, 2>{ 1.0f, 1.0f });
        result.setNormalUVSet(mat["normalUV"] ? mat["normalUV"]->numberValue(0) : 0);
    }
    
    // Fresnel
    if (mat["fresnel"] && mat["fresnel"]->isColor())
    {
        result.setFresnelTint(mat["fresnel"]->colorValue());
    }
    
    // Sheen
    if (mat["sheenIntensity"] && mat["sheenIntensity"]->isNumber())
    {
        result.setSheenIntensity(mat["sheenIntensity"]->numberValue());
    }
    if (mat["sheenColor"] && mat["sheenColor"]->isColor())
    {
        result.setSheenColor(mat["sheenColor"]->colorValue());
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
    std::vector<std::shared_ptr<base::Texture>> & uniqueTextures,
    bool relativePaths
) {
    using namespace bg2e::json;
    
    auto matJson = JSON(JsonObject{
        { "name", JSON(mat.name()) },
        { "groupName", JSON(mat.groupName()) },
        { "type", JSON("pbr") },
        { "class", JSON("PBRMaterial") },
        { "isTransparent", JSON(mat.isTransparent()) },
        { "isSolid", JSON(mat.isSolid() )},
        { "visible", JSON(mat.visible() )}
    });
    auto & obj = matJson->objectValue();
    
    if (mat.albedoTexture().get())
    {
        std::filesystem::path fileName = mat.albedoTexture()->imageFilePath();
        if (relativePaths)
        {
            fileName = fileName.filename();
        }
        obj["albedoTexture"] = JSON(fileName.string());
        addUniqueTexture(mat.albedoTexture(), uniqueTextures);
        obj["albedoScale"] = JSON(mat.albedoScale());
        obj["albedoUV"] = JSON(mat.albedoUVSet());
    }
    
    obj["albedo"] = JSON(mat.albedo());
    
    if (mat.metalnessTexture().get())
    {
        std::filesystem::path fileName = mat.metalnessTexture()->imageFilePath();
        if (relativePaths)
        {
            fileName = fileName.filename();
        }
        obj["metalnessTexture"] = JSON(fileName.string());
        addUniqueTexture(mat.metalnessTexture(), uniqueTextures);
        obj["metallicChannel"] = JSON(mat.metalnessChannel());
        obj["metallicScale"] = JSON(mat.metalnessScale());
        obj["metallicUV"] = JSON(mat.metalnessUVSet());
    }
    
    obj["metalness"] = JSON(mat.metalness());
    
    
    if (mat.roughnessTexture().get())
    {
        std::filesystem::path fileName = mat.roughnessTexture()->imageFilePath();
        if (relativePaths)
        {
            fileName = fileName.filename();
        }
        obj["roughnessTexture"] = JSON(fileName.string());
        addUniqueTexture(mat.roughnessTexture(), uniqueTextures);
        obj["roughnessChannel"] = JSON(mat.roughnessChannel());
        obj["roughnessScale"] = JSON(mat.roughnessScale());
        obj["roughnessUV"] = JSON(mat.roughnessUVSet());
    }
    
    
    obj["roughness"] = JSON(mat.roughness());
    
    
    if (mat.normalTexture().get())
    {
        std::filesystem::path fileName = mat.normalTexture()->imageFilePath();
        if (relativePaths)
        {
            fileName = fileName.filename();
        }
        obj["normalTexture"] = JSON(fileName.string());
        addUniqueTexture(mat.normalTexture(), uniqueTextures);
        obj["normalScale"] = JSON(mat.normalScale());
        obj["normalUV"] = JSON(mat.normalUVSet());
    }
    
    obj["fresnel"] = JSON(mat.fresnelTint());
    
    obj["sheenIntensity"] = JSON(mat.sheenIntensity());
    obj["sheenColor"] = JSON(mat.sheenColor());
    
    if (mat.aoTexture().get())
    {
        std::filesystem::path fileName = mat.aoTexture()->imageFilePath();
        if (relativePaths)
        {
            fileName = fileName.filename();
        }
        obj["ambientOcclussion"] = JSON(fileName.string());
        addUniqueTexture(mat.aoTexture(), uniqueTextures);
        obj["ambientOcclussionScale"] = JSON(mat.aoScale());
        obj["ambientOcclussionChannel"] = JSON(mat.aoChannel());
        obj["ambientOcclussionUV"] = JSON(mat.aoUVSet());
    }
    
    return matJson->serialize();
}

std::string MaterialSerializer::serializeMaterialArray(
    std::vector<base::MaterialAttributes>& mat,
    std::vector<std::shared_ptr<base::Texture>> & uniqueTextures,
    bool relativePaths
) {
    std::string result = "[";
    std::string sep = "";
    for (auto & m : mat)
    {
        result += sep + serializeMaterial(m, uniqueTextures, relativePaths);
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
