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

#pragma once

#include <bg2e/base/MaterialAttributes.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/draw/Drawable.hpp>

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

    // Live overloads use the CPU patch/return/callback policy. Call at a stopped
    // scene/GPU boundary. updateTextures uses the target's cache policy and flushes
    // pending texture slots; failures propagate after any already applied edits.
    // Direct material edits affect live attributes only. Drawable edits patch the
    // stored materialDefinition and live attributes separately, without copying
    // a whole material. The caller requests redraw/history reset (for example via
    // DefaultRenderLoopDelegate::enqueueRendererUpdate) after edits, also on failure.
    bool apply(draw::MaterialBase& material, TextureLoadedCallback callback = {}) const;
    bool apply(const std::shared_ptr<draw::MaterialBase>& material,
               TextureLoadedCallback callback = {}) const;

    bool apply(draw::Drawable& drawable, uint32_t submeshIndex,
               TextureLoadedCallback callback = {}) const;
    bool apply(draw::Drawable& drawable, const std::string& submeshName,
               TextureLoadedCallback callback = {}) const;

    std::size_t applyAll(draw::Drawable& drawable, TextureLoadedCallback callback = {}) const;
    std::size_t applyAll(draw::Drawable& drawable, const std::string& groupName,
                         TextureLoadedCallback callback = {}) const;
    std::size_t applyAll(draw::Drawable& drawable, const std::regex& groupPattern,
                         TextureLoadedCallback callback = {}) const;

  private:
    struct Data;
    std::shared_ptr<const Data> _data;
};

}
