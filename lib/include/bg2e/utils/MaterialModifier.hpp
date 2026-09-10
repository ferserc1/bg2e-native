#pragma once

#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/scene/Drawable.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <regex>
#include <string>

namespace bg2e::utils {

// A reusable snapshot of a partial PBR material definition. Missing, null and
// incorrectly typed properties are ignored. Submesh metadata is never changed.
class BG2E_API MaterialModifier {
public:
    using TextureLoadedCallback = std::function<void()>;

    explicit MaterialModifier(const std::string& jsonString,
        const std::filesystem::path& basePath = {});
    explicit MaterialModifier(const std::shared_ptr<json::JsonNode>& jsonData,
        const std::filesystem::path& basePath = {});

    [[nodiscard]] bool isValid() const;

    // CPU only: texture setters mark their slots dirty for subsequent loading.
    bool apply(base::MaterialAttributes& material) const;

    // Call on the render thread, at the same safe point as MaterialEditor.
    // Uses the target's existing engine and texture-cache policy. Texture-load
    // exceptions propagate; GPU updates are not transactional. No ownership is
    // retained. Direct MaterialBase edits do not synchronize an owning Drawable.
    bool apply(render::MaterialBase& material, TextureLoadedCallback callback = {}) const;
    bool apply(const std::shared_ptr<render::MaterialBase>& material,
        TextureLoadedCallback callback = {}) const;

    // Patch CPU and live attributes independently. No whole-material copy and
    // no updateMaterials() call, which could overwrite other runtime edits.
    bool apply(scene::Drawable& drawable, uint32_t submeshIndex,
        TextureLoadedCallback callback = {}) const;

    // Apply to the submesh whose name exactly matches submeshName.
    bool apply(scene::Drawable& drawable, const std::string& submeshName,
        TextureLoadedCallback callback = {}) const;

    // Return the number of successfully selected/applied submeshes (including
    // valid no-op patches), or zero for an invalid modifier/no matches.
    std::size_t applyAll(scene::Drawable& drawable, TextureLoadedCallback callback = {}) const;
    std::size_t applyAll(scene::Drawable& drawable, const std::string& groupName,
        TextureLoadedCallback callback = {}) const;
    // Uses regex_search, equivalent to JavaScript RegExp.test, not regex_match.
    std::size_t applyAll(scene::Drawable& drawable, const std::regex& groupPattern,
        TextureLoadedCallback callback = {}) const;

private:
    struct Data;
    std::shared_ptr<const Data> _data;
};

}
