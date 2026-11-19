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

#include <optional>
#include <string>

#include <bg2e/scene/DrawableRenderComponent.hpp>
#include <bg2e/draw/DrawableCache.hpp>
#include <bg2e/draw/Drawable.hpp>
#include <utility>

namespace bg2e {
namespace scene {

class BG2E_API DrawableComponent : public DrawableRenderComponent {
  public:
    BG2E_COMPONENT_TYPE_NAME("Drawable");

    DrawableComponent();
    DrawableComponent(std::shared_ptr<bg2e::draw::Drawable> drw);
    DrawableComponent(bg2e::draw::Drawable* drw);
    DrawableComponent(std::nullptr_t);
    DrawableComponent(const DrawableComponent& other);
    DrawableComponent& operator=(const DrawableComponent& other);
    ~DrawableComponent() override;

    // Editor overlays may reuse DrawableComponent storage without representing
    // authored scene geometry. Node::drawable excludes those helpers.
    bool isSceneGeometry() const override
    {
        return true;
    }

    // Procedural authored geometry serializes its own data, never a baked model.
    virtual bool requiresBg2Export() const
    {
        return true;
    }

    bool renderable(bool forPicking = false) const override;
    std::shared_ptr<bg2e::draw::Drawable> drawable();
    std::shared_ptr<const bg2e::draw::Drawable> drawable() const override;
    // Replacement follows the current resource identity, notifying all instances.
    [[nodiscard]] draw::DrawableCacheDiagnostic setDrawable(std::shared_ptr<draw::Drawable> drw);
    [[nodiscard]] draw::DrawableCacheDiagnostic setDrawable(std::nullptr_t)
    {
        return clearDrawable();
    }
    [[nodiscard]] draw::DrawableCacheDiagnostic clearDrawable();
    // Explicitly adopt a different identity. Never hijacks another owner.
    [[nodiscard]] draw::DrawableCacheDiagnostic rebindResource(draw::DrawableCache& cache,
        const draw::DrawableResourceHandle& handle);
    const draw::DrawableResourceHandle& resourceHandle() const noexcept;
    const draw::DrawableCacheDiagnostic& assignmentDiagnostic() const noexcept;
    static bool prepareResources(Node* root, draw::Engine& engine) noexcept;

    std::shared_ptr<Component> clone() const override;

    // Resolved source reference; setting it does not replace live resources.
    void setResourcePath(const std::filesystem::path& path);
    // Exchange already-normalized references without allocation (transaction rollback).
    void swapResourcePath(std::filesystem::path& path) noexcept
    {
        _resourcePath.swap(path);
    }
    bool hasPendingExport() const noexcept
    {
        return !_pendingExportPath.empty();
    }
    std::filesystem::path sourcePath() const;
    const std::filesystem::path& resourcePath() const
    {
        return _resourcePath;
    }
    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) override;
    // Snapshot a prepared publication reference, preserving custom serializers
    // and restoring authored references on every exit path.
    std::shared_ptr<json::JsonNode> serializeExportReferences(const std::filesystem::path& basePath,
        const std::filesystem::path& modelPath);
    void collectDependencyIssues(const ComponentValidationContext& context,
        std::vector<ComponentIssue>& issues) const override;

    void loadResources(bg2e::draw::Engine& engine, SceneLoadProgress* progress = nullptr) override;

    // Reference-only representation. Complete export requires engine-aware DB IO.
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

  protected:
    std::shared_ptr<draw::DrawableUsageLease> retainFrameResources() const override;

    std::filesystem::path _resourcePath;
    std::optional<std::string> _resourceName;
    std::shared_ptr<bg2e::draw::Drawable> _drawable;

  private:
    std::shared_ptr<const void> _ownerToken;
    std::shared_ptr<draw::DrawableCache> _cacheContext;
    draw::DrawableResourceHandle _handle;
    draw::DrawableUsageLease _ownerUse;
    draw::DrawableCacheDiagnostic _assignmentDiagnostic;
    bool _independentCopy = false;
    bool _explicitlyCleared = false;
    bool _copyAttempted = false;
    std::filesystem::path _registrationSourcePath;
    std::filesystem::path _pendingExportPath;
    std::filesystem::path _copySourcePath;
    std::string _resourceWarning;

    draw::DrawableCacheDiagnostic report(draw::DrawableCacheDiagnostic diagnostic);
    draw::DrawableCacheDiagnostic registerResource(draw::DrawableCache& cache);
    void assignIndependentPath();
    void publishIndependentCopy() noexcept;
    void adoptPreparedCopy(DrawableComponent& prepared);
};

}
}
