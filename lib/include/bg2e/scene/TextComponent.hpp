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

#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/base/TextTextureGenerator.hpp>

namespace bg2e::scene {

// One XY quad, facing +Z, +X tangent, +Y up. Both UV sets run top-left to
// bottom-right. Ordinary material appearance is independent of paragraph data.
class BG2E_API TextComponent final : public DrawableComponent {
  public:
    void collectDependencyIssues(const ComponentValidationContext& context,
        std::vector<ComponentIssue>& issues) const override;

    BG2E_COMPONENT_TYPE_NAME("Text");

    std::optional<ComponentIssue> checkCompatibility(const Component& other) const override;

    TextComponent();

    bool requiresBg2Export() const override;
    base::Text& text();
    const base::Text& text() const;
    void setText(const base::Text& value) noexcept;

    // World units: height=1 by default; width follows logical paragraph aspect.
    // setSize selects manual dimensions. Resolution never changes world size.
    glm::vec2 size() const noexcept;
    // Unmodified authored pair, including the dormant manual width in aspect mode.
    glm::vec2 authoredSize() const noexcept
    {
        return _size;
    }

    void setSize(const glm::vec2& value) noexcept;
    void setHeight(float value) noexcept;
    bool preserveAspect() const noexcept;
    void setPreserveAspect(bool value) noexcept;
    glm::vec2 pivot() const noexcept;
    // Normalized bottom-left origin, default center (0.5,0.5).
    void setPivot(const glm::vec2& value) noexcept;

    base::MaterialAttributes& material();
    const base::MaterialAttributes& material() const;
    // Staged CPU replacement, preserving the owned source. No GPU work here.
    void setMaterial(const base::MaterialAttributes& value) noexcept;

    bool resourcesDirty() const noexcept;
    size_t retainedCpuBytes() const noexcept;
    bool hasQuadResources() const noexcept;
    bool canRender() const noexcept;
    void retryResources() noexcept;
    // Stopped old-engine boundary, before changing/destroying the draw context.
    // Keeps independent CPU paragraph/material data for later realization.
    void releaseResources() noexcept;
    // Caller stops frame/background producers. No active recording/acquisition.
    // False means unchanged; failures warn and suppress this text, not the scene.
    bool prepareResources(draw::Engine& engine) noexcept;
    static bool prepareResources(Node* root, draw::Engine& engine) noexcept;
    void reportRenderFailure() noexcept;

    void update(float delta) override;
    void loadResources(draw::Engine& engine, SceneLoadProgress* progress = nullptr) noexcept override;
    std::shared_ptr<Component> clone() const noexcept override;
    // Representation only: embedded paragraph and conventional material object.
    // Albedo source references are ignored; only its UV/sampling data is restored.
    void deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath) noexcept override;
    std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) noexcept override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) noexcept override;

  protected:
    void didAddToNode(Node* owner) override;
    void didRemoveFromNode(Node* owner) override;

  private:
    // Hide model-backed conveniences in the direct API, not through base casts.
    using DrawableComponent::setDrawable;
    using DrawableComponent::clearDrawable;
    using DrawableComponent::setResourcePath;
    using DrawableComponent::swapResourcePath;

    base::Text _text;
    base::MaterialAttributes _fallbackMaterial;
    std::optional<base::MaterialAttributes> _pendingMaterial;
    std::shared_ptr<base::TextTextureGenerator> _generator;
    std::shared_ptr<const base::TextFont> _font;
    glm::vec2 _size{2, 1}, _pivot{0.5f};
    bool _preserveAspect = true;
    uint64_t _editVersion = 1;

    draw::Engine* _attemptEngine = nullptr;
    Scene* _attemptScene = nullptr;
    uint64_t _attemptText = 0, _attemptFonts = 0, _attemptEdit = 0, _attemptMaterial = 0;
    glm::vec2 _attemptSize{0}, _attemptPivot{0};
    glm::vec2 _realizedSize{0}, _realizedPivot{0};
    bool _attempted = false, _ready = false, _hasInk = false;

    std::shared_ptr<draw::Drawable::Mesh> makeQuad() const;
    void checkpoint(draw::Engine& engine, const std::shared_ptr<const base::TextFont>& font) noexcept;
};

}
