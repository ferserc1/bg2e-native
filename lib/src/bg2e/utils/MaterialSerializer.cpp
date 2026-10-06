/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/utils/MaterialSerializer.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/json/NodeReader.hpp>

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
    json::JsonNode* node,
    const std::filesystem::path& basePath,
    base::MaterialAttributes& result
)
{
    if (!node) return false;
    json::ObjectReader reader(*node);
    if (!reader.isValid()) return false;

    if (auto value = reader.getString("name")) result.setName(*value);
    if (auto value = reader.getString("groupName")) result.setGroupName(*value);
    if (auto value = reader.getBool("isTransparent")) result.setIsTransparent(*value);
    if (auto value = reader.getNumber("refractionFactor")) result.setRefractionFactor(*value);
    if (auto value = reader.getNumber("alphaCutoff")) result.setAlphaCutoff(*value);
    if (auto value = reader.getBool("isSolid")) result.setIsSolid(*value);
    if (auto value = reader.getBool("visible")) result.setVisible(*value);
    if (auto value = reader.getBool("isUnlit")) result.setIsUnlit(*value);

    if (auto value = reader.getVec4("albedo")) result.setAlbedo(*value);
    else if (auto value = reader.getVec3("albedo")) result.setAlbedo(*value);

    const auto texture = [&](const char* key) -> base::Texture* {
        auto file = reader.getString(key);
        return file ? getTexture(basePath, *file) : nullptr;
    };
    const auto scale = [&](const char* key) {
        return reader.getVec2(key).value_or(std::array<float, 2>{1.0f, 1.0f});
    };
    const auto index = [&](const char* key, uint32_t fallback = 0) {
        return reader.getInteger<uint32_t>(key).value_or(fallback);
    };

    if (auto value = texture("albedoTexture"))
    {
        result.setAlbedoTexture(value);
        result.setAlbedoScale(scale("albedoScale"));
        result.setAlbedoUVSet(index("albedoUV"));
    }
    if (auto value = reader.getNumber("metalness")) result.setMetalness(*value);
    if (auto value = texture("metalnessTexture"))
    {
        result.setMetalnessTexture(value);
        result.setMetalnessChannel(index("metalnessChannel"));
        result.setMetalnessScale(scale("metalnessScale"));
        result.setMetalnessUVSet(index("metalnessUV"));
    }
    if (auto value = reader.getNumber("roughness")) result.setRoughness(*value);
    if (auto value = texture("roughnessTexture"))
    {
        result.setRoughnessTexture(value);
        result.setRoughnessChannel(index("roughnessChannel"));
        result.setRoughnessScale(scale("roughnessScale"));
        result.setRoughnessUVSet(index("roughnessUV"));
    }
    if (auto value = texture("normalTexture"))
    {
        result.setNormalTexture(value);
        result.setNormalScale(scale("normalScale"));
        result.setNormalUVSet(index("normalUV"));
    }
    if (auto value = reader.getColor("fresnelTint")) result.setFresnelTint(*value);
    if (auto value = reader.getNumber("sheenIntensity")) result.setSheenIntensity(*value);
    if (auto value = reader.getColor("sheenColor")) result.setSheenColor(*value);
    if (auto value = texture("ambientOcclussion"))
    {
        result.setAoTexture(value);
        result.setAoScale(scale("ambientOcclussionScale"));
        result.setAoChannel(index("ambientOcclussionChannel"));
        result.setAoUVSet(index("ambientOcclussionUV", 1));
    }
    if (auto value = reader.getNumber("lightEmission")) result.setLightEmission(*value);
    if (auto value = texture("lightEmissionTexture")) result.setLightEmissionTexture(value);
    if (auto value = reader.getVec2("lightEmissionScale")) result.setLightEmissionScale(*value);
    if (auto value = reader.getInteger<uint32_t>("lightEmissionChannel")) result.setLightEmissionChannel(*value);
    if (auto value = reader.getBool("lightEmissionInvert")) result.setLightEmissionInvert(*value);
    if (auto value = reader.getInteger<uint32_t>("lightEmissionUV")) result.setLightEmissionUVSet(*value);
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
    // TODO: Maybe this can be set in other place
    json::JsonParser parser(jsonString);
    auto jsonData = parser.parse();
    json::ArrayReader reader(jsonData);
    if (!reader.isValid())
    {
        return false;
    }
    
    bool complete = true;
    for (std::size_t i = 0; i < reader.size(); ++i)
    {
        auto matItem = reader.getObject(i);
        base::MaterialAttributes mat;
        if (!matItem || !parseMaterial(matItem->node().get(), basePath, mat))
        {
            complete = false;
        }
        result.push_back(mat);
    }
    return complete;
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
        { "refractionFactor", JSON(mat.refractionFactor()) },
        { "alphaCutoff", JSON(mat.alphaCutoff()) },
        { "isSolid", JSON(mat.isSolid() )},
        { "visible", JSON(mat.visible() )},
        { "unlit", JSON(mat.isUnlit() )},
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
        obj["metalnessChannel"] = JSON(mat.metalnessChannel());
        obj["metalnessScale"] = JSON(mat.metalnessScale());
        obj["metalnessUV"] = JSON(mat.metalnessUVSet());
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
    
    obj["fresnelTint"] = JSON(mat.fresnelTint());
    
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
    
    obj["lightEmission"] = JSON(mat.lightEmission());
    obj["lightEmissionScale"] = JSON(mat.lightEmissionScale());
    obj["lightEmissionChannel"] = JSON(mat.lightEmissionChannel());
    obj["lightEmissionInvert"] = JSON(mat.lightEmissionInvert());
    obj["lightEmissionUV"] = JSON(mat.lightEmissionUVSet());

    if (mat.lightEmissionTexture().get())
    {
        std::filesystem::path fileName = mat.lightEmissionTexture()->imageFilePath();
        if (relativePaths)
        {
            fileName = fileName.filename();
        }
        obj["lightEmissionTexture"] = JSON(fileName.string());
        addUniqueTexture(mat.lightEmissionTexture(), uniqueTextures);
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
