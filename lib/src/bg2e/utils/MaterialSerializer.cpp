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
#include <stdexcept>
#include <memory>

namespace bg2e::utils {

base::Texture* getTexture(const std::filesystem::path& basePath, const std::string& file)
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
    base::MaterialAttributes& result)
{
    if (!node)
    {
        return false;
    }
    json::ObjectReader reader(*node);
    if (!reader.isValid())
    {
        return false;
    }

    if (auto value = reader.getString("indirectTextureMode"))
    {
        using Mode = base::MaterialAttributes::IndirectTextureMode;
        if (*value == "bakedAO")
        {
            result.setIndirectTextureMode(Mode::BakedAmbientOcclusion);
        }
        else if (*value == "bakedGI")
        {
            result.setIndirectTextureMode(Mode::BakedGlobalIllumination);
        }
        else if (*value == "ao")
        {
            result.setIndirectTextureMode(Mode::AmbientOcclusion);
        }
        else
        {
            throw std::invalid_argument("Unknown indirectTextureMode");
        }
    }

    if (auto value = reader.getString("name"))
    {
        result.setName(*value);
    }
    if (auto value = reader.getString("groupName"))
    {
        result.setGroupName(*value);
    }
    if (auto value = reader.getBool("isTransparent"))
    {
        result.setIsTransparent(*value);
    }
    if (auto value = reader.getNumber("refractionFactor"))
    {
        result.setRefractionFactor(*value);
    }
    if (auto value = reader.getNumber("alphaCutoff"))
    {
        result.setAlphaCutoff(*value);
    }
    if (auto value = reader.getBool("doubleSided"))
    {
        result.setDoubleSided(*value);
    }
    if (auto value = reader.getBool("isSolid"))
    {
        result.setIsSolid(*value);
    }
    if (auto value = reader.getBool("visible"))
    {
        result.setVisible(*value);
    }
    if (auto value = reader.getBool("isUnlit").has_value() ? reader.getBool("isUnlit") : reader.getBool("unlit"))
    {
        result.setIsUnlit(*value);
    }

    if (auto value = reader.getVec4("albedo"))
    {
        result.setAlbedo(*value);
    }
    else if (auto value = reader.getVec3("albedo"))
    {
        result.setAlbedo(*value);
    }

    const auto texture = [&](const char* key, const std::shared_ptr<base::Texture>& existing) -> base::Texture*
    {
        auto file = reader.getString(key);
        auto settings = reader.getObject(std::string(key) + "Settings");
        if (!file && (!settings || !existing))
        {
            return nullptr;
        }
        // A procedural owner supplies the source. Restore sampling without
        // inventing an empty file reference or deserializing ownership policy.
        auto value = file ? std::unique_ptr<base::Texture>(getTexture(basePath, *file)) :
            std::make_unique<base::Texture>(*existing);
        if (settings)
        {
            const auto mag = settings->getInteger<uint32_t>("mag").value_or(value->magFilter());
            const auto min = settings->getInteger<uint32_t>("min").value_or(value->minFilter());
            const auto u = settings->getInteger<uint32_t>("u").value_or(value->addressModeU());
            const auto v = settings->getInteger<uint32_t>("v").value_or(value->addressModeV());
            const auto w = settings->getInteger<uint32_t>("w").value_or(value->addressModeW());
            const auto color = settings->getInteger<uint32_t>("color").value_or(value->colorType());
            if (mag > 1 || min > 1 || u > 3 || v > 3 || w > 3 || (color != 1 && color != 2))
            {
                throw std::invalid_argument("Invalid serialized texture settings");
            }
            value->setMagFilter(static_cast<base::Texture::Filter>(mag));
            value->setMinFilter(static_cast<base::Texture::Filter>(min));
            value->setAddressMode(static_cast<base::Texture::AddressMode>(u),
                static_cast<base::Texture::AddressMode>(v), static_cast<base::Texture::AddressMode>(w));
            value->setColorType(static_cast<base::Color::Type>(color));
            value->setUseMipmaps(settings->getBool("mipmaps").value_or(value->useMipmaps()));
            value->setMinLod(settings->getNumber("minLod").value_or(value->minLod()));
            value->setMaxLod(settings->getNumber("maxLod").value_or(value->maxLod()));
            value->setMipLodBias(settings->getNumber("bias").value_or(value->mipLodBias()));
        }
        return value.release();
    };
    const auto scale = [&](const char* key)
    {
        return reader.getVec2(key).value_or(std::array<float, 2>{1.0f, 1.0f});
    };
    const auto index = [&](const char* key, uint32_t fallback = 0)
    {
        return reader.getInteger<uint32_t>(key).value_or(fallback);
    };

    if (auto value = texture("albedoTexture", result.albedoTexture()))
    {
        result.setAlbedoTexture(value);
        result.setAlbedoScale(scale("albedoScale"));
        result.setAlbedoUVSet(index("albedoUV"));
    }
    if (auto value = reader.getNumber("metalness"))
    {
        result.setMetalness(*value);
    }
    if (auto value = texture("metalnessTexture", result.metalnessTexture()))
    {
        result.setMetalnessTexture(value);
        result.setMetalnessChannel(index("metalnessChannel"));
        result.setMetalnessScale(scale("metalnessScale"));
        result.setMetalnessUVSet(index("metalnessUV"));
    }
    if (auto value = reader.getNumber("roughness"))
    {
        result.setRoughness(*value);
    }
    if (auto value = texture("roughnessTexture", result.roughnessTexture()))
    {
        result.setRoughnessTexture(value);
        result.setRoughnessChannel(index("roughnessChannel"));
        result.setRoughnessScale(scale("roughnessScale"));
        result.setRoughnessUVSet(index("roughnessUV"));
    }
    if (auto value = texture("normalTexture", result.normalTexture()))
    {
        result.setNormalTexture(value);
        result.setNormalScale(scale("normalScale"));
        result.setNormalUVSet(index("normalUV"));
    }
    if (auto value = reader.getBool("metalnessInvert"))
    {
        result.setMetalnessInvert(*value);
    }
    if (auto value = reader.getBool("roughnessInvert"))
    {
        result.setRoughnessInvert(*value);
    }
    if (auto value = reader.getColor("fresnelTint"))
    {
        result.setFresnelTint(*value);
    }
    if (auto value = reader.getNumber("sheenIntensity"))
    {
        result.setSheenIntensity(*value);
    }
    if (auto value = reader.getColor("sheenColor"))
    {
        result.setSheenColor(*value);
    }
    if (auto value = texture("ambientOcclussion", result.aoTexture()))
    {
        if (result.indirectTextureMode() != base::MaterialAttributes::IndirectTextureMode::AmbientOcclusion)
        {
            value->setColorType(base::Color::TypeLinear);
        }
        result.setAoTexture(value);
        result.setAoScale(scale("ambientOcclussionScale"));
        result.setAoChannel(index("ambientOcclussionChannel"));
        result.setAoUVSet(index("ambientOcclussionUV", 1));
    }
    if (auto value = reader.getNumber("lightEmission"))
    {
        result.setLightEmission(*value);
    }
    if (auto value = texture("lightEmissionTexture", result.lightEmissionTexture()))
    {
        result.setLightEmissionTexture(value);
    }
    if (auto value = reader.getVec2("lightEmissionScale"))
    {
        result.setLightEmissionScale(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("lightEmissionChannel"))
    {
        result.setLightEmissionChannel(*value);
    }
    if (auto value = reader.getBool("lightEmissionInvert"))
    {
        result.setLightEmissionInvert(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("lightEmissionUV"))
    {
        result.setLightEmissionUVSet(*value);
    }
    // UV/scalar controls are material data even when a source is omitted. This
    // also permits deserializing them before a procedural owner attaches pixels.
    if (auto value = reader.getVec2("albedoScale"))
    {
        result.setAlbedoScale(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("albedoUV"))
    {
        result.setAlbedoUVSet(*value);
    }
    if (auto value = reader.getVec2("metalnessScale"))
    {
        result.setMetalnessScale(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("metalnessUV"))
    {
        result.setMetalnessUVSet(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("metalnessChannel"))
    {
        result.setMetalnessChannel(*value);
    }
    if (auto value = reader.getVec2("roughnessScale"))
    {
        result.setRoughnessScale(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("roughnessUV"))
    {
        result.setRoughnessUVSet(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("roughnessChannel"))
    {
        result.setRoughnessChannel(*value);
    }
    if (auto value = reader.getVec2("normalScale"))
    {
        result.setNormalScale(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("normalUV"))
    {
        result.setNormalUVSet(*value);
    }
    if (auto value = reader.getVec2("ambientOcclussionScale"))
    {
        result.setAoScale(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("ambientOcclussionUV"))
    {
        result.setAoUVSet(*value);
    }
    if (auto value = reader.getInteger<uint32_t>("ambientOcclussionChannel"))
    {
        result.setAoChannel(*value);
    }
    return true;
}

bool MaterialSerializer::deserializeMaterial(
    const std::string& jsonString,
    const std::filesystem::path& basePath,
    base::MaterialAttributes& result)
{
    json::JsonParser parser(jsonString);
    auto jsonData = parser.parse();

    return parseMaterial(jsonData.get(), basePath, result);
}

bool MaterialSerializer::deserializeMaterialArray(
    const std::string& jsonString,
    const std::filesystem::path& basePath,
    std::vector<base::MaterialAttributes>& result)
{
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
    std::vector<std::shared_ptr<base::Texture>>& uniqueTextures,
    bool relativePaths)
{
    using namespace bg2e::json;

    auto matJson = JSON(JsonObject{
        {"name", JSON(mat.name())},
        {"groupName", JSON(mat.groupName())},
        {"type", JSON("pbr")},
        {"class", JSON("PBRMaterial")},
        {"isTransparent", JSON(mat.isTransparent())},
        {"refractionFactor", JSON(mat.refractionFactor())},
        {"alphaCutoff", JSON(mat.alphaCutoff())},
        {"isSolid", JSON(mat.isSolid())},
        {"visible", JSON(mat.visible())},
        {"isUnlit", JSON(mat.isUnlit())},
        {"doubleSided", JSON(mat.doubleSided())},
    });
    auto& obj = matJson->objectValue();
    const auto source = [&](const char* key, const std::shared_ptr<base::Texture>& texture)
    {
        if (!texture || texture->ignoreSerialization())
        {
            return;
        }
        std::filesystem::path fileName = texture->imageFilePath();
        if (relativePaths)
        {
            fileName = fileName.filename();
        }
        obj[key] = JSON(fileName.string());
        addUniqueTexture(texture, uniqueTextures);
    };

    if (mat.albedoTexture().get())
    {
        source("albedoTexture", mat.albedoTexture());
        obj["albedoScale"] = JSON(mat.albedoScale());
        obj["albedoUV"] = JSON(mat.albedoUVSet());
    }

    obj["albedo"] = JSON(mat.albedo());

    if (mat.metalnessTexture().get())
    {
        source("metalnessTexture", mat.metalnessTexture());
        obj["metalnessChannel"] = JSON(mat.metalnessChannel());
        obj["metalnessScale"] = JSON(mat.metalnessScale());
        obj["metalnessUV"] = JSON(mat.metalnessUVSet());
    }

    obj["metalness"] = JSON(mat.metalness());
    obj["metalnessInvert"] = JSON(mat.metalnessInvert());

    if (mat.roughnessTexture().get())
    {
        source("roughnessTexture", mat.roughnessTexture());
        obj["roughnessChannel"] = JSON(mat.roughnessChannel());
        obj["roughnessScale"] = JSON(mat.roughnessScale());
        obj["roughnessUV"] = JSON(mat.roughnessUVSet());
    }

    obj["roughness"] = JSON(mat.roughness());
    obj["roughnessInvert"] = JSON(mat.roughnessInvert());

    if (mat.normalTexture().get())
    {
        source("normalTexture", mat.normalTexture());
        obj["normalScale"] = JSON(mat.normalScale());
        obj["normalUV"] = JSON(mat.normalUVSet());
    }

    obj["fresnelTint"] = JSON(mat.fresnelTint());

    obj["sheenIntensity"] = JSON(mat.sheenIntensity());
    obj["sheenColor"] = JSON(mat.sheenColor());

    using IndirectMode = base::MaterialAttributes::IndirectTextureMode;
    obj["indirectTextureMode"] = JSON(std::string(mat.indirectTextureMode() == IndirectMode::BakedGlobalIllumination ? "bakedGI" :
        mat.indirectTextureMode() == IndirectMode::BakedAmbientOcclusion ? "bakedAO" : "ao"));

    if (mat.aoTexture().get())
    {
        source("ambientOcclussion", mat.aoTexture());
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
        source("lightEmissionTexture", mat.lightEmissionTexture());
    }

    const auto settings = [&](const char* key, const std::shared_ptr<base::Texture>& texture)
    {
        if (texture)
        {
            obj[std::string(key) + "Settings"] = JSON(JsonObject{
                {"mag", JSON(static_cast<uint32_t>(texture->magFilter()))},
                {"min", JSON(static_cast<uint32_t>(texture->minFilter()))},
                {"u", JSON(static_cast<uint32_t>(texture->addressModeU()))},
                {"v", JSON(static_cast<uint32_t>(texture->addressModeV()))},
                {"w", JSON(static_cast<uint32_t>(texture->addressModeW()))},
                {"color", JSON(static_cast<uint32_t>(texture->colorType()))},
                {"mipmaps", JSON(texture->useMipmaps())},
                {"minLod", JSON(texture->minLod())}, {"maxLod", JSON(texture->maxLod())},
                {"bias", JSON(texture->mipLodBias())}});
        }
    };
    settings("albedoTexture", mat.albedoTexture());
    settings("metalnessTexture", mat.metalnessTexture());
    settings("roughnessTexture", mat.roughnessTexture());
    settings("normalTexture", mat.normalTexture());
    settings("ambientOcclussion", mat.aoTexture());
    settings("lightEmissionTexture", mat.lightEmissionTexture());

    return matJson->serialize();
}

std::string MaterialSerializer::serializeMaterialArray(
    std::vector<base::MaterialAttributes>& mat,
    std::vector<std::shared_ptr<base::Texture>>& uniqueTextures,
    bool relativePaths)
{
    std::string result = "[";
    std::string sep = "";
    for (auto& m : mat)
    {
        result += sep + serializeMaterial(m, uniqueTextures, relativePaths);
        sep = ",";
    }
    return result + "]";
}

void MaterialSerializer::addUniqueTexture(
    std::shared_ptr<base::Texture> tex,
    std::vector<std::shared_ptr<base::Texture>>& textures)
{
    // Only add textures associated with a file
    if (!tex || tex->ignoreSerialization() || tex->imageFilePath().empty())
    {
        return;
    }

    for (auto& t : textures)
    {
        if (t->imageFilePath() == tex->imageFilePath())
        {
            return;
        }
    }

    textures.push_back(tex);
}

}
