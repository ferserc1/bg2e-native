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

#include <bg2e/scene/Component.hpp>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace bg2e::scene {

// Declarations only; font bytes belong to Scene's registry, never this component.
// Any node may declare fonts, but identifiers are unique across the whole scene.
class BG2E_API TextFontComponent final : public Component {
  public:
    BG2E_COMPONENT_TYPE_NAME("TextFonts");

    struct Declaration {
        std::string identifier;
        std::filesystem::path path;
        uint32_t faceIndex = 0;
        // Optional supplied license/provenance files; no redistribution rights
        // are inferred. Complete scene saves copy these alongside the font.
        std::vector<std::filesystem::path> notices{};
        bool operator==(const Declaration&) const = default;
    };
    static constexpr size_t MaxDeclarations = 128;

    TextFontComponent() = default;

    const std::vector<Declaration>& declarations() const;
    void setDeclarations(std::vector<Declaration> declarations) noexcept;
    void addFont(std::string identifier, std::filesystem::path path, uint32_t faceIndex = 0);
    void removeFont(const std::string& identifier);
    // Invalid declarations warn and are skipped; bad face indices default to 0.
    static bool validateDeclaration(const Declaration& declaration) noexcept;

    std::shared_ptr<Component> clone() const noexcept override;
    void deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath) noexcept override;
    std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) noexcept override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) noexcept override;
    void loadResources(draw::Engine& engine, SceneLoadProgress* progress = nullptr) noexcept override;

  protected:
    void didAddToNode(Node* owner) override;
    void didRemoveFromNode(Node* owner) override;

  private:
    std::vector<Declaration> _declarations;
};

}
