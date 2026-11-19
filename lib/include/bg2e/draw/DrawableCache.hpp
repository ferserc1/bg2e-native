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

#include <bg2e/draw/Drawable.hpp>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace bg2e::draw {

class DrawableCache;

// CPU-only dependency values. Readers must not retain their component payload.
struct DrawableDependencies {
    std::vector<std::filesystem::path> files;
    std::vector<std::filesystem::path> models;
    std::vector<std::string> warnings;
};

BG2E_API void appendMaterialDependencies(const base::MaterialAttributes& material, DrawableDependencies& result);

// One logical use; moving transfers registration, destruction releases it.
// History/jobs/frames additionally pin the realization acquired at registration.
// Frame registrations must live until GPU completion, not just command recording.
// Scene owners/instances resolve the handle each time to follow replacement.
class BG2E_API DrawableUsageLease {
  public:
    DrawableUsageLease() = default;
    ~DrawableUsageLease() = default;
    DrawableUsageLease(DrawableUsageLease&&) noexcept = default;
    DrawableUsageLease& operator=(DrawableUsageLease&&) noexcept = default;
    DrawableUsageLease(const DrawableUsageLease&) = delete;
    DrawableUsageLease& operator=(const DrawableUsageLease&) = delete;

    bool valid() const noexcept;
    std::shared_ptr<const Drawable> snapshot() const noexcept;
    uint64_t geometryRevision() const noexcept;

  private:
    friend class DrawableCache;
    std::shared_ptr<detail::DrawableUsageRecord> _record;
};

enum class DrawableCacheCode
{
    Success,
    InvalidArgument,
    NotFound,
    ForeignResource,
    CrossDevice,
    InUse,
    Busy,
    Collision,
    LoadFailed,
    ResourceFailed
};

struct [[nodiscard]] DrawableCacheDiagnostic {
    DrawableCacheCode code = DrawableCacheCode::Success;
    std::string message;

    bool ok() const noexcept
    {
        return code == DrawableCacheCode::Success;
    }
};

struct [[nodiscard]] DrawableCacheResult {
    DrawableResourceHandle handle;
    DrawableCacheDiagnostic diagnostic;

    explicit operator bool() const noexcept
    {
        return diagnostic.ok();
    }
};

struct [[nodiscard]] DrawableUsageResult {
    DrawableUsageLease lease;
    DrawableCacheDiagnostic diagnostic;

    explicit operator bool() const noexcept
    {
        return diagnostic.ok();
    }
};

enum class DrawableResourceKind
{
    Bg2,
    Procedural,
    Imported
};

// Published file basis relative to the resident local geometry. This is export
// bookkeeping, never a source lookup alias or a mutation of original defaults.
struct DrawableExportPublication {
    std::filesystem::path path;
    std::vector<glm::mat4> submeshTransforms;
};

struct DrawableCacheEntryInfo {
    DrawableResourceHandle handle;
    DrawableResourceKind kind = DrawableResourceKind::Procedural;
    std::string displayName;
    std::filesystem::path sourcePath;
    std::filesystem::path exportPath;
    std::vector<std::filesystem::path> lookupAliases;
    uint64_t geometryRevision = 0;
    glm::mat4 originalTransform{1};
    std::shared_ptr<const std::vector<Drawable::SubmeshAttributes>> originalSubmeshes;
    DrawableUsage usage;
    std::vector<DrawableExportPublication> publishedExports;
};

// Engine-owned, strongly resident and independent of scenes, DB and UI. Like
// ImageLoadContext, use on the serialized resource-update thread. Mutations need
// stopped frame/background producers; loader callbacks may query but not mutate
// the cache. Repeated normalized BG2 requests return the same resident entry.
// Engine cleanup invalidates all handles/leases before destroying its device.
// Release separately extracted native resource references before engine cleanup;
// every long-lived consumer must hold a lease, not just a resolve() snapshot.
class BG2E_API DrawableCache : public std::enable_shared_from_this<DrawableCache> {
  public:
    using Loader = std::function<std::shared_ptr<Drawable>(const std::filesystem::path&, Engine&)>;

    ~DrawableCache();
    DrawableCache(const DrawableCache&) = delete;
    DrawableCache& operator=(const DrawableCache&) = delete;

    Engine& engine() const noexcept;
    bool active() const noexcept;
    uint64_t scopeIdentity() const noexcept;
    uint64_t contentGeneration() const noexcept;
    DrawableDependencies dependencies() const;

    DrawableCacheResult load(const std::filesystem::path& bg2Path, const Loader& loader);
    DrawableCacheResult find(const std::filesystem::path& bg2Path) const;
    DrawableCacheResult findIdentifier(const std::string& identifier) const;
    DrawableCacheResult registerProcedural(std::shared_ptr<Drawable> resource, const std::string& displayName);
    DrawableCacheResult registerImported(std::shared_ptr<Drawable> resource,
        const std::filesystem::path& sourcePath, const std::string& displayName);
    // Keeps identity, aliases and source/export metadata; publishes a new geometry
    // revision and independent original definitions only after full realization.
    DrawableCacheResult replace(const DrawableResourceHandle& handle, std::shared_ptr<Drawable> resource);

    // Publish in-place owner geometry changes without replacing the identity.
    DrawableCacheResult refresh(const DrawableResourceHandle& handle);
    // Restore immutable defaults from this identity's retained History version,
    // after the owner has restored that exact realization.
    DrawableCacheResult restoreOriginalDefinitions(const DrawableResourceHandle& handle, const DrawableUsageLease& history);

    std::vector<DrawableCacheEntryInfo> entries() const;
    std::optional<DrawableCacheEntryInfo> inspect(const DrawableResourceHandle& handle) const;
    // Internal rendering/owner bridge. Instances must not expose this Drawable:
    // its legacy mesh() accessor is mutable even through a const Drawable.
    std::shared_ptr<const Drawable> resolve(const DrawableResourceHandle& handle) const noexcept;
    DrawableUsage queryUsage(const DrawableResourceHandle& handle) const noexcept;
    DrawableUsageResult retain(const DrawableResourceHandle& handle, DrawableUse use, const std::string& debugIdentifier);
    // Called at forced engine shutdown, before the device dies. Capture a weak
    // payload, never a raw component pointer. The callback must not throw.
    DrawableUsageResult retain(const DrawableResourceHandle& handle, DrawableUse use,
        const std::string& debugIdentifier, std::function<void()> releaseDeviceResources);
    // Additional independent appearance sources, including retained history.
    DrawableUsageResult retain(const DrawableResourceHandle& handle, DrawableUse use,
        const std::string& debugIdentifier, std::function<void()> releaseDeviceResources,
        std::function<DrawableDependencies()> dependencies);

    DrawableCacheDiagnostic canUnload(const DrawableResourceHandle& handle) const;
    DrawableCacheResult unload(const DrawableResourceHandle& handle);
    // Transactional refusal if any entry is used. Zero-use entries stay resident
    // until an explicit unload/clear; handle copies do not prevent unloading.
    DrawableCacheResult clear();

    // Export destinations never implicitly become lookup aliases (Save As).
    DrawableCacheResult setExportPath(const DrawableResourceHandle& handle, const std::filesystem::path& path);
    DrawableCacheResult recordExportPublication(const DrawableResourceHandle& handle,
        const std::filesystem::path& path, const std::vector<glm::mat4>& submeshTransforms);
    DrawableCacheResult addLookupAlias(const DrawableResourceHandle& handle, const std::filesystem::path& bg2Path);

  private:
    friend class Engine;
    explicit DrawableCache(Engine& engine);

    struct Impl;
    std::unique_ptr<Impl> _impl;

    DrawableCacheResult publish(std::shared_ptr<Drawable> resource, DrawableResourceKind kind,
        const std::string& identifier, const std::string& displayName, const std::filesystem::path& sourcePath);
    std::shared_ptr<detail::DrawableCacheEntry> ownedEntry(const DrawableResourceHandle& handle) const noexcept;
    DrawableResourceHandle handleFor(const std::shared_ptr<detail::DrawableCacheEntry>& entry) const;
    static void releaseRealization(Drawable& resource) noexcept;
    void releaseResources() noexcept;
    void shutdown() noexcept;
};

}
