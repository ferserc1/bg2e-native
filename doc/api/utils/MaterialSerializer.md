# MaterialSerializer

**Header:** `<bg2e/utils/MaterialSerializer.hpp>` · **Namespace:** `bg2e::utils`

The conventional PBR JSON dialect shared by model resources, material editing and
TextComponent's embedded material object. There is no text-specific color/material
dialect and no nested JSON string in the saved Text component.

```cpp
bool deserializeMaterial(const std::string& jsonString,
    const std::filesystem::path& basePath, base::MaterialAttributes& result);
bool deserializeMaterial(const char* jsonString,
    const std::filesystem::path& basePath, base::MaterialAttributes& result);
bool deserializeMaterialArray(const std::string& jsonString,
    const std::filesystem::path& basePath, std::vector<base::MaterialAttributes>& result);
bool deserializeMaterialArray(const char* jsonString,
    const std::filesystem::path& basePath, std::vector<base::MaterialAttributes>& result);
std::string serializeMaterial(base::MaterialAttributes& mat,
    std::vector<std::shared_ptr<base::Texture>>& uniqueTextures, bool relativePaths);
std::string serializeMaterialArray(std::vector<base::MaterialAttributes>& mat,
    std::vector<std::shared_ptr<base::Texture>>& uniqueTextures, bool relativePaths);
```

Scalar appearance includes `doubleSided` (default false in MaterialAttributes),
alpha cutoff, unlit and conventional PBR values. Texture source keys are
albedoTexture, metalnessTexture, roughnessTexture, normalTexture,
ambientOcclussion (existing spelling) and lightEmissionTexture. Corresponding
`<key>Settings` objects contain mag/min/u/v/w/color/mipmaps/minLod/maxLod/bias;
UV/scale/channel/inversion are independent material fields.

Textures marked ignoreSerialization omit their source key and uniqueTextures
entry, but retain settings and UV/scale controls. A procedural owner **seeds its
regenerated source in result before deserialization**: settings-only records
restore onto that source, not an invented empty-path texture. No sourceEditable/
ignoreSerialization permission is imported from JSON. Missing slots stay absent.

Generic relativePaths=true writes basenames; owners such as TextComponent rewrite
full filesystem references lexically against their target directory. The serializer
does not copy images/fonts or perform transactional publication. Complete
[resource IO](../db/DrawResourceIO.md#editable-text-and-font-assets) handles that.
TextComponent strips incoming albedo source references **before** invoking this
parser and establishes its owned-source flags separately.

These generic parser methods are not noexcept and can reject malformed enums or
allocate/throw. A false result is not a valid representation; array parsing may
already have appended entries. TextComponent wraps this lower-level contract with
warning/default/recovery handling so one bad text cannot abort scene loading.
Do not claim all material parsing is globally nonthrowing because text is safe.
