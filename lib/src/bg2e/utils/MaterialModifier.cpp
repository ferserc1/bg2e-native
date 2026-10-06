#include <bg2e/utils/MaterialModifier.hpp>
#include <bg2e/json/NodeReader.hpp>
#include "MaterialModifierJson.hpp"

#include <cmath>
#include <limits>
#include <optional>

namespace bg2e::utils {
namespace {

template <typename T>
std::optional<T> field(const json::ObjectReader& reader, const char* key)
{
    if constexpr (std::is_same_v<T, float>)
    {
        auto value = reader.getNumber(key);
        return value && std::isfinite(*value) ? value : std::nullopt;
    }
    else if constexpr (std::is_same_v<T, bool>) return reader.getBool(key);
    else if constexpr (std::is_same_v<T, uint32_t>) return reader.getInteger<uint32_t>(key);
    else if constexpr (std::is_same_v<T, glm::vec2>)
    {
        auto value = reader.getGlmVec2(key);
        return value && std::isfinite(value->x) && std::isfinite(value->y) ? value : std::nullopt;
    }
    else if constexpr (std::is_same_v<T, base::Color>)
    {
        if (auto value = reader.getColor(key)) return value;
        if (auto value = reader.getVec3(key))
            return base::Color((*value)[0], (*value)[1], (*value)[2], 1.0f);
        return std::nullopt;
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        auto value = reader.getString(key);
        return value && !value->empty() ? value : std::nullopt;
    }
}

std::shared_ptr<base::Texture> texture(const std::filesystem::path& basePath,
    const std::string& filename)
{
    auto result = std::make_shared<base::Texture>();
    result->setImageFilePath((basePath / filename).lexically_normal().string());
    result->setMagFilter(base::Texture::FilterLinear);
    result->setMinFilter(base::Texture::FilterLinear);
    result->setUseMipmaps(true);
    return result;
}

} // namespace

struct MaterialModifier::Data {
    std::filesystem::path basePath;
    std::optional<float> refractionFactor;
    std::optional<float> metalness;
    std::optional<float> roughness;
    std::optional<float> sheenIntensity;
    std::optional<float> lightEmission;
    std::optional<bool> isTransparent;
    std::optional<bool> isSolid;
    std::optional<bool> unlit;
    std::optional<bool> metalnessInvert;
    std::optional<bool> roughnessInvert;
    std::optional<bool> lightEmissionInvert;
    std::optional<uint32_t> albedoUV;
    std::optional<uint32_t> metalnessChannel;
    std::optional<uint32_t> metalnessUV;
    std::optional<uint32_t> roughnessChannel;
    std::optional<uint32_t> roughnessUV;
    std::optional<uint32_t> normalUV;
    std::optional<uint32_t> ambientOcclussionChannel;
    std::optional<uint32_t> ambientOcclussionUV;
    std::optional<uint32_t> lightEmissionChannel;
    std::optional<uint32_t> lightEmissionUV;
    std::optional<glm::vec2> albedoScale;
    std::optional<glm::vec2> normalScale;
    std::optional<glm::vec2> metalnessScale;
    std::optional<glm::vec2> roughnessScale;
    std::optional<glm::vec2> ambientOcclussionScale;
    std::optional<glm::vec2> lightEmissionScale;
    std::optional<base::Color> albedo;
    std::optional<base::Color> fresnelTint;
    std::optional<base::Color> sheenColor;
    std::optional<std::string> albedoTexture;
    std::optional<std::string> normalTexture;
    std::optional<std::string> metalnessTexture;
    std::optional<std::string> roughnessTexture;
    std::optional<std::string> ambientOcclussion;
    std::optional<std::string> lightEmissionTexture;
};

MaterialModifier::MaterialModifier(const std::string& text, const std::filesystem::path& path)
{
    try
    {
        auto node = detail::MaterialModifierJson(text).parse();
        _data = MaterialModifier(node, path)._data;
    }
    catch (const std::logic_error&) {}
}

MaterialModifier::MaterialModifier(const std::shared_ptr<json::JsonNode>& node,
    const std::filesystem::path& path)
{
    json::ObjectReader reader(node);
    if (!reader.isValid()) return;
    if (!reader.isUndefined("class") && reader.getString("class").value_or("") != "PBRMaterial") return;

    auto data = std::make_shared<Data>();
    data->basePath = path;
    data->refractionFactor = field<float>(reader, "refractionFactor");
    data->metalness = field<float>(reader, "metalness");
    data->roughness = field<float>(reader, "roughness");
    data->sheenIntensity = field<float>(reader, "sheenIntensity");
    data->lightEmission = field<float>(reader, "lightEmission");
    data->isTransparent = field<bool>(reader, "isTransparent");
    data->isSolid = field<bool>(reader, "isSolid");
    data->unlit = field<bool>(reader, "unlit");
    data->metalnessInvert = field<bool>(reader, "metalnessInvert");
    data->roughnessInvert = field<bool>(reader, "roughnessInvert");
    data->lightEmissionInvert = field<bool>(reader, "lightEmissionInvert");
    data->albedoUV = field<uint32_t>(reader, "albedoUV");
    data->metalnessChannel = field<uint32_t>(reader, "metalnessChannel");
    data->metalnessUV = field<uint32_t>(reader, "metalnessUV");
    data->roughnessChannel = field<uint32_t>(reader, "roughnessChannel");
    data->roughnessUV = field<uint32_t>(reader, "roughnessUV");
    data->normalUV = field<uint32_t>(reader, "normalUV");
    data->ambientOcclussionChannel = field<uint32_t>(reader, "ambientOcclussionChannel");
    data->ambientOcclussionUV = field<uint32_t>(reader, "ambientOcclussionUV");
    data->lightEmissionChannel = field<uint32_t>(reader, "lightEmissionChannel");
    data->lightEmissionUV = field<uint32_t>(reader, "lightEmissionUV");
    data->albedoScale = field<glm::vec2>(reader, "albedoScale");
    data->normalScale = field<glm::vec2>(reader, "normalScale");
    data->metalnessScale = field<glm::vec2>(reader, "metalnessScale");
    data->roughnessScale = field<glm::vec2>(reader, "roughnessScale");
    data->ambientOcclussionScale = field<glm::vec2>(reader, "ambientOcclussionScale");
    data->lightEmissionScale = field<glm::vec2>(reader, "lightEmissionScale");
    data->albedo = field<base::Color>(reader, "albedo");
    data->fresnelTint = field<base::Color>(reader, "fresnelTint");
    data->sheenColor = field<base::Color>(reader, "sheenColor");
    data->albedoTexture = field<std::string>(reader, "albedoTexture");
    data->normalTexture = field<std::string>(reader, "normalTexture");
    data->metalnessTexture = field<std::string>(reader, "metalnessTexture");
    data->roughnessTexture = field<std::string>(reader, "roughnessTexture");
    data->ambientOcclussion = field<std::string>(reader, "ambientOcclussion");
    data->lightEmissionTexture = field<std::string>(reader, "lightEmissionTexture");
    // Shader channel indices and UV sets must remain in their supported range.
    if (data->albedoUV && *data->albedoUV > 1) data->albedoUV.reset();
    if (data->metalnessChannel && *data->metalnessChannel > 3) data->metalnessChannel.reset();
    if (data->metalnessUV && *data->metalnessUV > 1) data->metalnessUV.reset();
    if (data->roughnessChannel && *data->roughnessChannel > 3) data->roughnessChannel.reset();
    if (data->roughnessUV && *data->roughnessUV > 1) data->roughnessUV.reset();
    if (data->normalUV && *data->normalUV > 1) data->normalUV.reset();
    if (data->ambientOcclussionChannel && *data->ambientOcclussionChannel > 3) data->ambientOcclussionChannel.reset();
    if (data->ambientOcclussionUV && *data->ambientOcclussionUV > 1) data->ambientOcclussionUV.reset();
    if (data->lightEmissionChannel && *data->lightEmissionChannel > 3) data->lightEmissionChannel.reset();
    if (data->lightEmissionUV && *data->lightEmissionUV > 1) data->lightEmissionUV.reset();
    _data = std::move(data);
}

bool MaterialModifier::isValid() const { return static_cast<bool>(_data); }

bool MaterialModifier::apply(base::MaterialAttributes& material) const
{
    if (!_data) return false;
    const auto& data = *_data;
    if (data.refractionFactor) material.setRefractionFactor(*data.refractionFactor);
    if (data.metalness) material.setMetalness(*data.metalness);
    if (data.roughness) material.setRoughness(*data.roughness);
    if (data.sheenIntensity) material.setSheenIntensity(*data.sheenIntensity);
    if (data.lightEmission) material.setLightEmission(*data.lightEmission);
    if (data.isTransparent) material.setIsTransparent(*data.isTransparent);
    if (data.isSolid) material.setIsSolid(*data.isSolid);
    if (data.unlit) material.setIsUnlit(*data.unlit);
    if (data.metalnessInvert) material.setMetalnessInvert(*data.metalnessInvert);
    if (data.roughnessInvert) material.setRoughnessInvert(*data.roughnessInvert);
    if (data.lightEmissionInvert) material.setLightEmissionInvert(*data.lightEmissionInvert);
    if (data.albedoUV) material.setAlbedoUVSet(*data.albedoUV);
    if (data.metalnessChannel) material.setMetalnessChannel(*data.metalnessChannel);
    if (data.metalnessUV) material.setMetalnessUVSet(*data.metalnessUV);
    if (data.roughnessChannel) material.setRoughnessChannel(*data.roughnessChannel);
    if (data.roughnessUV) material.setRoughnessUVSet(*data.roughnessUV);
    if (data.normalUV) material.setNormalUVSet(*data.normalUV);
    if (data.ambientOcclussionChannel) material.setAoChannel(*data.ambientOcclussionChannel);
    if (data.ambientOcclussionUV) material.setAoUVSet(*data.ambientOcclussionUV);
    if (data.lightEmissionChannel) material.setLightEmissionChannel(*data.lightEmissionChannel);
    if (data.lightEmissionUV) material.setLightEmissionUVSet(*data.lightEmissionUV);
    if (data.albedoScale) material.setAlbedoScale(*data.albedoScale);
    if (data.normalScale) material.setNormalScale(*data.normalScale);
    if (data.metalnessScale) material.setMetalnessScale(*data.metalnessScale);
    if (data.roughnessScale) material.setRoughnessScale(*data.roughnessScale);
    if (data.ambientOcclussionScale) material.setAoScale(*data.ambientOcclussionScale);
    if (data.lightEmissionScale) material.setLightEmissionScale(*data.lightEmissionScale);
    if (data.albedo) material.setAlbedo(*data.albedo);
    if (data.fresnelTint) material.setFresnelTint(*data.fresnelTint);
    if (data.sheenColor) material.setSheenColor(*data.sheenColor);
    if (data.albedoTexture) material.setAlbedoTexture(texture(data.basePath, *data.albedoTexture));
    if (data.normalTexture) material.setNormalTexture(texture(data.basePath, *data.normalTexture));
    if (data.metalnessTexture) material.setMetalnessTexture(texture(data.basePath, *data.metalnessTexture));
    if (data.roughnessTexture) material.setRoughnessTexture(texture(data.basePath, *data.roughnessTexture));
    if (data.ambientOcclussion) material.setAoTexture(texture(data.basePath, *data.ambientOcclussion));
    if (data.lightEmissionTexture) material.setLightEmissionTexture(texture(data.basePath, *data.lightEmissionTexture));
    return true;
}

bool MaterialModifier::apply(render::MaterialBase& material, TextureLoadedCallback callback) const
{
    if (!apply(material.materialAttributes())) return false;
    material.updateTextures(std::move(callback));
    return true;
}

bool MaterialModifier::apply(const std::shared_ptr<render::MaterialBase>& material,
    TextureLoadedCallback callback) const
{
    return material && apply(*material, std::move(callback));
}

bool MaterialModifier::apply(scene::Drawable& drawable, uint32_t index,
    TextureLoadedCallback callback) const
{
    if (!_data || index >= drawable.submeshesCount()) return false;
    // Resolve the runtime target before touching CPU data.
    auto live = drawable.isLoaded() ? drawable.renderMaterial(index) : nullptr;
    if (drawable.isLoaded() && !live) return false;
    apply(drawable.material(index));
    if (live) apply(live, std::move(callback));
    return true;
}

bool MaterialModifier::apply(scene::Drawable& drawable, const std::string& submeshName,
    TextureLoadedCallback callback) const
{
    for (uint32_t i = 0; i < drawable.submeshesCount(); ++i)
    {
        if (drawable.submeshName(i) == submeshName)
            return apply(drawable, i, std::move(callback));
    }
    return false;
}

std::size_t MaterialModifier::applyAll(scene::Drawable& drawable,
    TextureLoadedCallback callback) const
{
    std::size_t count = 0;
    for (uint32_t i = 0; i < drawable.submeshesCount(); ++i)
        if (apply(drawable, i, callback)) ++count;
    return count;
}

std::size_t MaterialModifier::applyAll(scene::Drawable& drawable, const std::string& group,
    TextureLoadedCallback callback) const
{
    std::size_t count = 0;
    for (uint32_t i = 0; i < drawable.submeshesCount(); ++i)
        if (drawable.submeshGroupName(i) == group && apply(drawable, i, callback)) ++count;
    return count;
}

std::size_t MaterialModifier::applyAll(scene::Drawable& drawable, const std::regex& pattern,
    TextureLoadedCallback callback) const
{
    std::size_t count = 0;
    for (uint32_t i = 0; i < drawable.submeshesCount(); ++i)
        if (std::regex_search(drawable.submeshGroupName(i), pattern) &&
            apply(drawable, i, callback)) ++count;
    return count;
}

}
