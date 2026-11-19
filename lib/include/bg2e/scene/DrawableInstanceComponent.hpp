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

#include <bg2e/scene/DrawableRenderComponent.hpp>
#include <bg2e/draw/DrawableCache.hpp>

namespace bg2e::scene {

// Shared immutable geometry, independently authored appearance and transforms.
// Bind/prepare/release at a stopped engine resource-update boundary. The cache
// handle guards the borrowed cache pointer against engine teardown/recreation.
class BG2E_API DrawableInstanceComponent final : public DrawableRenderComponent {
  public:
    BG2E_COMPONENT_TYPE_NAME("DrawableInstance");

    DrawableInstanceComponent();
    ~DrawableInstanceComponent() override;

    [[nodiscard]] draw::DrawableCacheDiagnostic setResource(draw::DrawableCache& cache,
        const draw::DrawableResourceHandle& handle);
    const draw::DrawableResourceHandle& resourceHandle() const noexcept;
    std::filesystem::path resourcePath() const;
    [[nodiscard]] draw::DrawableCacheDiagnostic clearResource();
    bool prepareResources(draw::Engine& engine) noexcept;
    static bool prepareResources(Node* root, draw::Engine& engine) noexcept;
    [[nodiscard]] draw::DrawableCacheDiagnostic releaseResources();
    void retryResources() noexcept;
    void reportRenderFailure() noexcept;

    bool renderable(bool forPicking = false) const override;
    bool resourcesLoaded() const override;
    uint32_t retainedMaterialCount() const noexcept;
    uint32_t submeshesCount() const override;
    glm::mat4 transform() const override;
    void setTransform(const glm::mat4& value);
    glm::mat4 submeshTransform(uint32_t index) const override;
    glm::mat4 localSubmeshTransform(uint32_t index) const;
    void setSubmeshTransform(const glm::mat4& value, uint32_t index = 0);
    const std::string& submeshName(uint32_t index) const override;
    const std::string& submeshGroupName(uint32_t index) const override;
    bool submeshVisibility(uint32_t index) const override;
    void setSubmeshName(const std::string& value, uint32_t index = 0);
    void setSubmeshGroupName(const std::string& value, uint32_t index = 0);
    void setSubmeshVisibility(bool value, uint32_t index = 0);
    bool rayTracingEnabled() const override;
    void setRayTracingEnabled(bool value);

    base::MaterialAttributes& material(uint32_t index = 0);
    const base::MaterialAttributes& effectiveMaterial(uint32_t index) const override;
    void setMaterial(const base::MaterialAttributes& value, uint32_t index = 0);
    // A stopped edit updated both the authored definition and this instance's
    // retained native material. Keep its identity stable for history replay.
    void acknowledgeMaterialEdit(uint32_t index) noexcept;
    std::shared_ptr<draw::MaterialBase> drawMaterial(uint32_t index) const override;

    void update(float delta) override;
    void collectDependencyIssues(const ComponentValidationContext& context,
        std::vector<ComponentIssue>& issues) const override;

    void loadResources(draw::Engine& engine, SceneLoadProgress* progress = nullptr) override;
    std::shared_ptr<Component> clone() const override;
    // Detached appearance preparation for an owner replacement. The existing
    // stable handle is retained; no raster mesh or BLAS is copied.
    std::shared_ptr<DrawableInstanceComponent> cloneForResourceReplacement(const draw::Drawable& resource) const;
    bool synchronizeResourceRevision(const draw::Drawable& expected) noexcept;
    void deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;
    // Operation-local export view: never changes binding, materials or geometry.
    std::shared_ptr<json::JsonNode> serializeExportReferences(const std::filesystem::path& basePath,
        const std::filesystem::path& modelPath, const std::vector<base::MaterialAttributes>& materials,
        const std::vector<glm::mat4>& localTransforms) const;
    void recordExportPublication(const std::filesystem::path& path, const std::vector<glm::mat4>& basis);

  private:
    struct State;
    std::shared_ptr<State> _state;
    uint64_t _debugIdentity = 0;
    draw::DrawableCache* _cache = nullptr;
    draw::DrawableResourceHandle _handle;
    draw::DrawableUsageLease _use;
    glm::mat4 _transform{1};
    bool _tracing = true;
    uint64_t _revision = 0;
    bool _ready = false, _retry = true;
    std::string _failureKey;
    std::filesystem::path _resourcePath;
    std::vector<glm::mat4> _exportBasis;
    std::shared_ptr<json::JsonNode> _pendingAppearance;
    std::filesystem::path _appearanceBase;
    std::string _bindingFailure;

    draw::DrawableCacheDiagnostic bindResource(draw::DrawableCache& cache,
        const draw::DrawableResourceHandle& handle, bool restoreAppearance);
    void overlayAppearance(const std::shared_ptr<json::JsonNode>& data, const std::filesystem::path& base);
    void rememberField(const char* key, std::shared_ptr<json::JsonNode> value);
    void rememberSubmesh(uint32_t index, const char* key, std::shared_ptr<json::JsonNode> value);

    // Deliberately inaccessible on the direct API and empty via a base cast.
    std::shared_ptr<const draw::Drawable> drawable() const override;
    std::shared_ptr<const draw::Drawable> renderResource() const override;
    std::shared_ptr<draw::DrawableUsageLease> retainFrameResources() const override;
    bool needsPreparation() const;
    std::string preparationKey() const;
    void warn(const char* message) const noexcept;
};

}
