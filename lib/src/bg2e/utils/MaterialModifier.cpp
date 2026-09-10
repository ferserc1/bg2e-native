#include <bg2e/utils/MaterialModifier.hpp>
#include "MaterialModifierJson.hpp"

#include <cmath>
#include <limits>
#include <optional>

namespace bg2e::utils {
namespace {

template <typename T>
std::optional<T> read(json::JsonNode& node);

template <> std::optional<float> read(json::JsonNode& node)
{
    if (node.isNumber() && std::isfinite(node.numberValue())) return node.numberValue();
    return {};
}

template <> std::optional<bool> read(json::JsonNode& node)
{
    if (node.isBool()) return node.boolValue();
    return {};
}

template <> std::optional<uint32_t> read(json::JsonNode& node)
{
    const auto value = read<float>(node);
    if (value && *value >= 0 && std::floor(*value) == *value &&
        static_cast<double>(*value) <= std::numeric_limits<uint32_t>::max())
        return static_cast<uint32_t>(*value);
    return {};
}

bool numericArray(json::JsonNode& node, std::size_t size)
{
    if (!node.isList() || node.listValue().size() != size) return false;
    for (const auto& element : node.listValue())
        if (!element || !read<float>(*element)) return false;
    return true;
}

template <> std::optional<glm::vec2> read(json::JsonNode& node)
{
    if (numericArray(node, 2)) return node.glmVec2Value();
    return {};
}

template <> std::optional<base::Color> read(json::JsonNode& node)
{
    if (numericArray(node, 4)) return node.colorValue();
    if (numericArray(node, 3))
    {
        auto v = node.vec3Value();
        return base::Color(v[0], v[1], v[2], 1.0f);
    }
    return {};
}

template <> std::optional<std::string> read(json::JsonNode& node)
{
    // Empty paths are not a texture removal request.
    if (node.isString() && !node.stringValue().empty()) return node.stringValue();
    return {};
}

template <typename T>
std::optional<T> field(const json::JsonObject& object, const char* key)
{
    auto it = object.find(key);
    return it != object.end() && it->second ? read<T>(*it->second) : std::nullopt;
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
    if (!node || !node->isObject()) return;
    const auto& object = node->objectValue();
    auto cls = object.find("class");
    if (cls != object.end() && (!cls->second || !cls->second->isString() ||
        cls->second->stringValue() != "PBRMaterial")) return;

    auto data = std::make_shared<Data>();
    data->basePath = path;
    data->refractionFactor = field<float>(object, "refractionFactor");
    data->metalness = field<float>(object, "metalness");
    data->roughness = field<float>(object, "roughness");
    data->sheenIntensity = field<float>(object, "sheenIntensity");
    data->lightEmission = field<float>(object, "lightEmission");
    data->isTransparent = field<bool>(object, "isTransparent");
    data->isSolid = field<bool>(object, "isSolid");
    data->unlit = field<bool>(object, "unlit");
    data->metalnessInvert = field<bool>(object, "metalnessInvert");
    data->roughnessInvert = field<bool>(object, "roughnessInvert");
    data->lightEmissionInvert = field<bool>(object, "lightEmissionInvert");
    data->albedoUV = field<uint32_t>(object, "albedoUV");
    data->metalnessChannel = field<uint32_t>(object, "metalnessChannel");
    data->metalnessUV = field<uint32_t>(object, "metalnessUV");
    data->roughnessChannel = field<uint32_t>(object, "roughnessChannel");
    data->roughnessUV = field<uint32_t>(object, "roughnessUV");
    data->normalUV = field<uint32_t>(object, "normalUV");
    data->ambientOcclussionChannel = field<uint32_t>(object, "ambientOcclussionChannel");
    data->ambientOcclussionUV = field<uint32_t>(object, "ambientOcclussionUV");
    data->lightEmissionChannel = field<uint32_t>(object, "lightEmissionChannel");
    data->lightEmissionUV = field<uint32_t>(object, "lightEmissionUV");
    data->albedoScale = field<glm::vec2>(object, "albedoScale");
    data->normalScale = field<glm::vec2>(object, "normalScale");
    data->metalnessScale = field<glm::vec2>(object, "metalnessScale");
    data->roughnessScale = field<glm::vec2>(object, "roughnessScale");
    data->ambientOcclussionScale = field<glm::vec2>(object, "ambientOcclussionScale");
    data->lightEmissionScale = field<glm::vec2>(object, "lightEmissionScale");
    data->albedo = field<base::Color>(object, "albedo");
    data->fresnelTint = field<base::Color>(object, "fresnelTint");
    data->sheenColor = field<base::Color>(object, "sheenColor");
    data->albedoTexture = field<std::string>(object, "albedoTexture");
    data->normalTexture = field<std::string>(object, "normalTexture");
    data->metalnessTexture = field<std::string>(object, "metalnessTexture");
    data->roughnessTexture = field<std::string>(object, "roughnessTexture");
    data->ambientOcclussion = field<std::string>(object, "ambientOcclussion");
    data->lightEmissionTexture = field<std::string>(object, "lightEmissionTexture");
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
