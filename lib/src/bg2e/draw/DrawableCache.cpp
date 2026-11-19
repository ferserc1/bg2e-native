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

#include <bg2e/draw/DrawableCache.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>

#include <algorithm>
#include <cstdio>
#include <atomic>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace bg2e::draw {
namespace detail {

struct DrawableUsageRecord {
    std::weak_ptr<DrawableCacheEntry> entry;
    std::shared_ptr<DrawableCache> context;
    DrawableUse use = DrawableUse::History;
    std::string debugIdentifier;
    std::shared_ptr<Drawable> realization;
    std::shared_ptr<const std::vector<Drawable::SubmeshAttributes>> originals;
    glm::mat4 originalTransform{1};
    uint64_t revision = 0;
    std::function<void()> releaseDeviceResources;
    std::function<DrawableDependencies()> dependencies;
};

struct DrawableCacheEntry {
    DrawableCache* cache = nullptr;
    bool resident = false;
    std::string identifier;
    DrawableResourceKind kind = DrawableResourceKind::Procedural;
    std::string displayName;
    std::filesystem::path sourcePath, exportPath;
    std::vector<std::filesystem::path> aliases;
    std::vector<DrawableExportPublication> publishedExports;
    std::shared_ptr<Drawable> resource;
    std::shared_ptr<const std::vector<Drawable::SubmeshAttributes>> originals;
    glm::mat4 originalTransform{1};
    uint64_t revision = 1;
    uint64_t geometryVersion = 0;
    std::vector<std::weak_ptr<DrawableUsageRecord>> uses;

    DrawableUsage usage() const noexcept
    {
        DrawableUsage result;
        result.registered = resident;
        if (!resident)
        {
            return result;
        }
        for (const auto& weak : uses)
        {
            if (const auto record = weak.lock())
            {
                switch (record->use)
                {
                case DrawableUse::SceneOwner: ++result.sceneOwners; break;
                case DrawableUse::SceneInstance: ++result.sceneInstances; break;
                case DrawableUse::History: ++result.history; break;
                case DrawableUse::Job: ++result.jobs; break;
                case DrawableUse::Frame: ++result.frames; break;
                }
            }
        }
        return result;
    }
};

}

namespace {

using Code = DrawableCacheCode;
using Entry = detail::DrawableCacheEntry;

DrawableCacheResult failure(Code code, const std::string& message)
{
    return {{}, {code, message}};
}

std::filesystem::path normalizedPath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        throw std::invalid_argument("Drawable resource path cannot be empty");
    }
    const auto result = std::filesystem::absolute(path).lexically_normal();
    if (result.filename().empty() || result.filename() == "." || result.filename() == "..")
    {
        throw std::invalid_argument("Drawable resource path must identify a file");
    }
    gpu::requireDebugName(result.generic_string());
    return result;
}

std::string uniqueSuffix()
{
    // Process-wide sequence distinguishes resources across multiple engines;
    // random session identity avoids reusing imported IDs after application restart.
    static const auto session = []
    {
        std::random_device random;
        std::ostringstream value;
        value << std::hex << std::setfill('0');
        for (int i = 0; i < 4; ++i)
        {
            value << std::setw(8) << random();
        }
        return value.str();
    }();
    static std::atomic<uint64_t> sequence{0};
    const auto next = sequence.fetch_add(1, std::memory_order_relaxed);
    if (next == std::numeric_limits<uint64_t>::max())
    {
        throw std::overflow_error("Drawable resource identity sequence exhausted");
    }
    return session + "-" + std::to_string(next);
}

std::shared_ptr<const std::vector<Drawable::SubmeshAttributes>> originalDefinitions(const Drawable& resource)
{
    auto result = std::make_shared<std::vector<Drawable::SubmeshAttributes>>();
    result->reserve(resource.submeshesCount());
    for (uint32_t i = 0; i < resource.submeshesCount(); ++i)
    {
        Drawable::SubmeshAttributes attributes;
        // Keep CPU source definitions independent of live material edits and of
        // Drawable::cleanup's snapshot of those edits back into materialDefinition.
        attributes.material = cloneMaterialAttributes(resource.materialDefinition(i));
        attributes.transform = resource.localSubmeshTransform(i);
        attributes.name = resource.submeshName(i);
        attributes.groupName = resource.submeshGroupName(i);
        attributes.visible = resource.submeshVisibility(i);
        result->push_back(std::move(attributes));
    }
    return result;
}

DrawableCacheDiagnostic validateRealization(const Drawable& resource, Engine& engine)
{
    const auto mesh = resource.drawMesh();
    if (!resource.isLoaded() || resource.engine() != &engine || !resource.mesh() || !mesh ||
        resource.mesh()->submeshes.size() != resource.submeshesCount() ||
        mesh->submeshCount() != resource.submeshesCount() ||
        resource.materials().size() != resource.submeshesCount())
    {
        return {Code::ResourceFailed, "Drawable realization is incomplete or has stale topology"};
    }
    if (!mesh->vertexBuffer() || !mesh->indexBuffer() ||
        mesh->vertexBuffer()->device() != engine.device() || mesh->indexBuffer()->device() != engine.device())
    {
        return {Code::CrossDevice, "Drawable raster buffers do not belong to this engine device"};
    }
    if (!mesh->vertexBuffer()->isValid() || !mesh->indexBuffer()->isValid())
    {
        return {Code::ResourceFailed, "Drawable raster buffers are unavailable"};
    }
    for (const auto& material : resource.materials())
    {
        if (!material || material->engine() != &engine)
        {
            return {Code::CrossDevice, "Drawable material is unavailable or belongs to another engine"};
        }
    }
    if (resource.rayTracingEnabled() && resource.rayTracingMeshes().size() != resource.submeshesCount())
    {
        return {Code::ResourceFailed, "Drawable tracing topology requires a reload"};
    }
    for (const auto& blas : resource.rayTracingMeshes())
    {
        if (!blas || blas->device() != engine.device())
        {
            return {Code::CrossDevice, "Drawable tracing geometry belongs to another device"};
        }
        if (blas->description().vertexBuffer != mesh->vertexBuffer() ||
            blas->description().indexBuffer != mesh->indexBuffer())
        {
            return {Code::ResourceFailed, "Drawable BLAS must borrow its retained raster buffers"};
        }
        gpu::requireDebugName(blas->description().debugName);
        if (!blas->isValid() || !blas->isBuilt())
        {
            return {Code::ResourceFailed, "Drawable tracing geometry has not been built"};
        }
    }
    return {};
}


// Resource mutations are serialized, including callbacks during realization.
// Reentrant loads cannot publish a second entry or deadlock waiting on themselves.
struct Mutation {
    explicit Mutation(bool& flag) : busy(flag)
    {
        busy = true;
    }
    ~Mutation()
    {
        busy = false;
    }
    bool& busy;
};

}

namespace {
std::atomic<uint64_t> nextCacheIdentity{1};
}

struct DrawableCache::Impl {
    Engine& engine;
    gpu::Device* device;
    bool busy = false;
    bool active = true;
    const uint64_t identity = nextCacheIdentity.fetch_add(1);
    uint64_t generation = 1;
    std::map<std::string, std::shared_ptr<Entry>> entries;
    // Includes displaced realizations pinned by history/jobs/frames. Shutdown
    // releases their native payloads before the engine destroys the device.
    std::vector<std::weak_ptr<Drawable>> realizations;

    explicit Impl(Engine& owner) : engine(owner), device(owner.device())
    {
        if (!device || !device->isValid())
        {
            throw std::invalid_argument("DrawableCache requires a live engine device");
        }
    }
};


DrawableUsage Drawable::cacheUsage() const noexcept
{
    const auto entry = _cacheEntry.lock();
    return entry ? entry->usage() : DrawableUsage{};
}


DrawableResourceHandle Drawable::resourceHandle() const
{
    const auto entry = _cacheEntry.lock();
    return entry && entry->resident ? entry->cache->findIdentifier(entry->identifier).handle : DrawableResourceHandle{};
}


std::shared_ptr<DrawableCache> DrawableResourceHandle::context() const noexcept
{
    const auto entry = _entry.lock();
    return entry && entry->resident ? entry->cache->weak_from_this().lock() : nullptr;
}


bool DrawableResourceHandle::valid() const noexcept
{
    const auto entry = _entry.lock();
    return entry && entry->resident;
}


const std::string& DrawableResourceHandle::identifier() const noexcept
{
    return _identifier;
}


uint64_t DrawableResourceHandle::geometryRevision() const noexcept
{
    const auto entry = _entry.lock();
    return entry && entry->resident ? entry->revision : 0;
}


bool DrawableResourceHandle::operator==(const DrawableResourceHandle& other) const noexcept
{
    // Expired handles from different caches must never compare equal merely
    // because the BG2 path is the same (or both weak pointers are expired).
    return !_entry.owner_before(other._entry) && !other._entry.owner_before(_entry);
}


bool DrawableUsageLease::valid() const noexcept
{
    const auto entry = _record ? _record->entry.lock() : nullptr;
    return entry && entry->resident;
}


std::shared_ptr<const Drawable> DrawableUsageLease::snapshot() const noexcept
{
    return valid() ? _record->realization : nullptr;
}


uint64_t DrawableUsageLease::geometryRevision() const noexcept
{
    return valid() ? _record->revision : 0;
}


DrawableCache::DrawableCache(Engine& engine) : _impl(std::make_unique<Impl>(engine))
{
}


DrawableCache::~DrawableCache()
{
    // A document scope may retire before the engine. Frame/job registrations
    // pin the context until their completion; drain before final native release.
    if (_impl->active && !_impl->entries.empty())
    {
        try
        {
            _impl->device->waitIdleForResourceUpdate();
        }
        catch (...)
        {
            std::fprintf(stderr, "Drawable context drain failed during retirement.\n");
        }
    }
    shutdown();
}


bool DrawableCache::active() const noexcept
{
    return _impl->active;
}


void DrawableCache::shutdown() noexcept
{
    if (!_impl->active)
    {
        return;
    }
    releaseResources();
    _impl->active = false;
}


Engine& DrawableCache::engine() const noexcept
{
    return _impl->engine;
}


std::shared_ptr<Entry> DrawableCache::ownedEntry(const DrawableResourceHandle& handle) const noexcept
{
    const auto entry = handle._entry.lock();
    return entry && entry->resident && entry->cache == this ? entry : nullptr;
}


DrawableResourceHandle DrawableCache::handleFor(const std::shared_ptr<Entry>& entry) const
{
    DrawableResourceHandle handle;
    handle._identifier = entry->identifier;
    handle._entry = entry;
    return handle;
}


DrawableCacheResult DrawableCache::find(const std::filesystem::path& bg2Path) const
try
{
    const auto path = normalizedPath(bg2Path);
    for (const auto& [id, entry] : _impl->entries)
    {
        if (std::find(entry->aliases.begin(), entry->aliases.end(), path) != entry->aliases.end())
        {
            return {handleFor(entry), {}};
        }
    }
    return failure(Code::NotFound, "Drawable source is not resident: " + path.generic_string());
}
catch (const std::exception& error)
{
    return failure(Code::InvalidArgument, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot resolve drawable source identity");
}


DrawableCacheResult DrawableCache::findIdentifier(const std::string& identifier) const
{
    const auto found = _impl->entries.find(identifier);
    return found == _impl->entries.end() ? failure(Code::NotFound, "Drawable identity is not resident") :
        DrawableCacheResult{handleFor(found->second), {}};
}



void appendMaterialDependencies(const base::MaterialAttributes& material, DrawableDependencies& result)
{
    for (const auto& texture : {material.albedoTexture(), material.metalnessTexture(), material.roughnessTexture(),
        material.normalTexture(), material.aoTexture(), material.lightEmissionTexture()})
    {
        if (texture && texture->type() == base::Texture::TypeFilesystem && !texture->imageFilePath().empty())
        {
            result.files.emplace_back(texture->imageFilePath());
        }
    }
}


uint64_t DrawableCache::scopeIdentity() const noexcept
{
    return _impl->identity;
}


uint64_t DrawableCache::contentGeneration() const noexcept
{
    return _impl->generation;
}


DrawableDependencies DrawableCache::dependencies() const
{
    DrawableDependencies result;
    const auto definitions = [&](const auto& values)
    {
        if (values)
        {
            for (const auto& value : *values)
            {
                appendMaterialDependencies(value.material, result);
            }
        }
    };
    const auto realization = [&](const Drawable& resource)
    {
        if (!resource.importSource().empty())
        {
            // Legacy OBJ provenance can identify its MTL rather than the OBJ.
            // Its current/original texture definitions are enumerated below.
            if (resource.importSource().extension() == ".mtl")
            {
                result.files.push_back(resource.importSource());
            }
            else
            {
                result.models.push_back(resource.importSource());
            }
        }
        for (uint32_t sub = 0; sub < resource.submeshesCount(); ++sub)
        {
            appendMaterialDependencies(resource.materialDefinition(sub), result);
            appendMaterialDependencies(resource.material(sub), result);
        }
    };
    for (const auto& [identifier, entry] : _impl->entries)
    {
        if (!entry->sourcePath.empty())
        {
            result.models.push_back(entry->sourcePath);
        }
        result.models.insert(result.models.end(), entry->aliases.begin(), entry->aliases.end());
        for (const auto& published : entry->publishedExports)
        {
            result.models.push_back(published.path);
        }
        definitions(entry->originals);
        realization(*entry->resource);
        for (const auto& weak : entry->uses)
        {
            if (const auto use = weak.lock())
            {
                definitions(use->originals);
                if (use->dependencies)
                {
                    // A failed reader aborts analysis rather than risking deletion.
                    const auto extra = use->dependencies();
                    result.files.insert(result.files.end(), extra.files.begin(), extra.files.end());
                    result.models.insert(result.models.end(), extra.models.begin(), extra.models.end());
                    result.warnings.insert(result.warnings.end(), extra.warnings.begin(), extra.warnings.end());
                }
            }
        }
    }
    for (const auto& weak : _impl->realizations)
    {
        if (const auto retained = weak.lock())
        {
            realization(*retained);
        }
    }
    return result;
}


DrawableCacheResult DrawableCache::load(const std::filesystem::path& bg2Path, const Loader& loader)
try
{
    const auto found = find(bg2Path);
    if (found || found.diagnostic.code != Code::NotFound)
    {
        if (found && _impl->active && !_impl->busy)
        {
            ++_impl->generation;
        }
        return found;
    }
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is already loading or updating a resource; retry after the current operation");
    }
    if (!loader)
    {
        return failure(Code::InvalidArgument, "A DB drawable loader is required");
    }
    Mutation mutation(_impl->busy);
    gpu::Device::ResourceUpdateScope boundary(*_impl->device);
    const auto path = normalizedPath(bg2Path);
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    if (extension != ".bg2" && extension != ".vwglb")
    {
        return failure(Code::InvalidArgument, "Cached BG2 loading requires a .bg2 or .vwglb source");
    }
    auto resource = loader(path, _impl->engine);
    return publish(std::move(resource), DrawableResourceKind::Bg2, path.generic_string(), path.filename().string(), path);
}
catch (const std::exception& error)
{
    return failure(Code::LoadFailed, error.what());
}
catch (...)
{
    return failure(Code::LoadFailed, "Cannot load drawable resource; no cache entry was published");
}


DrawableCacheResult DrawableCache::registerProcedural(std::shared_ptr<Drawable> resource, const std::string& displayName)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is already updating a resource");
    }
    Mutation mutation(_impl->busy);
    return publish(std::move(resource), DrawableResourceKind::Procedural, "procedural:" + uniqueSuffix(), displayName, {});
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot register procedural drawable");
}


DrawableCacheResult DrawableCache::registerImported(std::shared_ptr<Drawable> resource,
    const std::filesystem::path& sourcePath, const std::string& displayName)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is already updating a resource");
    }
    Mutation mutation(_impl->busy);
    const auto path = normalizedPath(sourcePath);
    return publish(std::move(resource), DrawableResourceKind::Imported,
        path.filename().string() + ":" + uniqueSuffix(), displayName, path);
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot register imported drawable");
}


DrawableCacheResult DrawableCache::publish(std::shared_ptr<Drawable> resource, DrawableResourceKind kind,
    const std::string& identifier, const std::string& displayName, const std::filesystem::path& sourcePath)
{
    gpu::requireDebugName(identifier);
    gpu::requireDebugName(displayName);
    if (!resource || !resource->mesh() || !resource->submeshesCount())
    {
        return failure(Code::InvalidArgument, "Drawable cache needs nonempty geometry and submesh definitions");
    }
    if (resource->engine() && resource->engine() != &_impl->engine)
    {
        return failure(Code::CrossDevice, "Drawable belongs to another engine/device; the previous cache state is unchanged");
    }
    if (resource->cacheUsage().registered)
    {
        return failure(Code::Collision, "Drawable already has a resource identity; clone it to create independent geometry");
    }
    if (_impl->entries.contains(identifier))
    {
        return failure(Code::Collision, "Drawable resource identity is already resident");
    }
    gpu::Device::ResourceUpdateScope boundary(*_impl->device);
    auto entry = std::make_shared<Entry>();
    entry->cache = this;
    entry->identifier = identifier;
    entry->kind = kind;
    entry->displayName = displayName;
    entry->sourcePath = sourcePath;
    if (kind == DrawableResourceKind::Bg2)
    {
        entry->aliases.push_back(sourcePath);
        entry->exportPath = sourcePath;
    }
    entry->originals = originalDefinitions(*resource);
    entry->originalTransform = resource->transform();
    entry->resource = resource;
    if (resource->name().empty())
    {
        resource->setName(displayName);
    }
    if (!resource->isLoaded())
    {
        resource->load(&_impl->engine);
    }
    const auto diagnostic = validateRealization(*resource, _impl->engine);
    if (!diagnostic.ok())
    {
        return {{}, diagnostic};
    }
    // Prepare every allocating result/tracking operation before publishing.
    auto handle = handleFor(entry);
    std::erase_if(_impl->realizations, [](const auto& weak) { return weak.expired(); });
    _impl->realizations.push_back(resource);
    _impl->entries.emplace(identifier, entry);
    resource->_cacheEntry = entry;
    entry->geometryVersion = resource->geometryVersion();
    entry->resident = true;
    ++_impl->generation;
    return {std::move(handle), {}};
}


DrawableCacheResult DrawableCache::replace(const DrawableResourceHandle& handle, std::shared_ptr<Drawable> resource)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is already updating a resource");
    }
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return failure(Code::ForeignResource, "Drawable handle is stale or belongs to another cache");
    }
    if (resource == entry->resource)
    {
        return {handle, {}};
    }
    if (!resource || !resource->mesh() || !resource->submeshesCount())
    {
        return failure(Code::InvalidArgument, "Replacement needs complete geometry and submesh definitions");
    }
    if (resource->engine() && resource->engine() != &_impl->engine)
    {
        return failure(Code::CrossDevice, "Replacement belongs to another engine/device");
    }
    if (resource->cacheUsage().registered && resource->_cacheEntry.lock() != entry)
    {
        return failure(Code::Collision, "Replacement already belongs to another resource; clone it before replacement");
    }
    const auto previousOwner = entry->resource->_ownerClaim.lock();
    const auto candidateOwner = resource->_ownerClaim.lock();
    if (previousOwner != candidateOwner && (previousOwner || candidateOwner))
    {
        return failure(Code::InUse, "Replace owned geometry through its DrawableComponent; resource ownership cannot be stolen");
    }
    if (entry->revision == std::numeric_limits<uint64_t>::max())
    {
        return failure(Code::ResourceFailed, "Drawable geometry revision exhausted");
    }
    Mutation mutation(_impl->busy);
    gpu::Device::ResourceUpdateScope boundary(*_impl->device);
    auto originals = originalDefinitions(*resource);
    const auto transform = resource->transform();
    if (resource->name().empty())
    {
        resource->setName(entry->displayName);
    }
    if (!resource->isLoaded())
    {
        resource->load(&_impl->engine);
    }
    const auto diagnostic = validateRealization(*resource, _impl->engine);
    if (!diagnostic.ok())
    {
        return {{}, diagnostic};
    }
    // Old queues/leases may retain raster buffers and BLAS together. Stop and
    // drain submissions before releasing this cache's old realization reference.
    _impl->device->waitIdleForResourceUpdate();
    auto resultHandle = handleFor(entry);
    std::erase_if(_impl->realizations, [](const auto& weak) { return weak.expired(); });
    _impl->realizations.push_back(resource);
    entry->resource = std::move(resource);
    entry->originals = std::move(originals);
    entry->originalTransform = transform;
    ++entry->revision;
    ++_impl->generation;
    entry->geometryVersion = entry->resource->geometryVersion();
    entry->resource->_cacheEntry = entry;
    return {std::move(resultHandle), {}};
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Drawable replacement failed; previous resource retained");
}


DrawableCacheResult DrawableCache::restoreOriginalDefinitions(const DrawableResourceHandle& handle, const DrawableUsageLease& history)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Cannot restore drawable defaults during another resource update");
    }
    const auto entry = ownedEntry(handle);
    const auto record = history._record;
    if (!entry || !record || record->use != DrawableUse::History || record->entry.lock() != entry ||
        record->realization != entry->resource || !record->originals || record->originals->size() != entry->resource->submeshesCount())
    {
        return failure(Code::InvalidArgument, "History defaults must match this identity and its restored realization");
    }
    if (entry->revision == std::numeric_limits<uint64_t>::max())
    {
        return failure(Code::ResourceFailed, "Drawable geometry revision exhausted");
    }
    auto result = handleFor(entry);
    entry->originals = record->originals;
    entry->originalTransform = record->originalTransform;
    ++entry->revision;
    ++_impl->generation;
    return {std::move(result), {}};
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot restore drawable original definitions");
}


DrawableCacheResult DrawableCache::refresh(const DrawableResourceHandle& handle)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is unavailable or changing resources");
    }
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return failure(Code::ForeignResource, "Drawable owner identity is no longer resident");
    }
    if (entry->geometryVersion == entry->resource->geometryVersion())
    {
        return {handle, {}};
    }
    const auto diagnostic = validateRealization(*entry->resource, _impl->engine);
    if (!diagnostic.ok())
    {
        return {{}, diagnostic};
    }
    if (entry->revision == std::numeric_limits<uint64_t>::max())
    {
        return failure(Code::ResourceFailed, "Drawable geometry revision exhausted");
    }
    auto originals = originalDefinitions(*entry->resource);
    auto result = handleFor(entry);
    entry->originals = std::move(originals);
    entry->originalTransform = entry->resource->transform();
    entry->geometryVersion = entry->resource->geometryVersion();
    ++entry->revision;
    ++_impl->generation;
    return {std::move(result), {}};
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot publish updated owner geometry definitions");
}


std::optional<DrawableCacheEntryInfo> DrawableCache::inspect(const DrawableResourceHandle& handle) const
{
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return std::nullopt;
    }
    return DrawableCacheEntryInfo{handleFor(entry), entry->kind, entry->displayName, entry->sourcePath,
        entry->exportPath, entry->aliases, entry->revision, entry->originalTransform, entry->originals, entry->usage(), entry->publishedExports};
}


std::vector<DrawableCacheEntryInfo> DrawableCache::entries() const
{
    std::vector<DrawableCacheEntryInfo> result;
    result.reserve(_impl->entries.size());
    for (const auto& [id, entry] : _impl->entries)
    {
        result.push_back(*inspect(handleFor(entry)));
    }
    return result;
}


std::shared_ptr<const Drawable> DrawableCache::resolve(const DrawableResourceHandle& handle) const noexcept
{
    const auto entry = ownedEntry(handle);
    return entry ? entry->resource : nullptr;
}


DrawableUsage DrawableCache::queryUsage(const DrawableResourceHandle& handle) const noexcept
{
    const auto entry = ownedEntry(handle);
    return entry ? entry->usage() : DrawableUsage{};
}


DrawableUsageResult DrawableCache::retain(const DrawableResourceHandle& handle, DrawableUse use, const std::string& debugIdentifier)
{
    return retain(handle, use, debugIdentifier, {});
}


DrawableUsageResult DrawableCache::retain(const DrawableResourceHandle& handle, DrawableUse use,
    const std::string& debugIdentifier, std::function<void()> releaseDeviceResources)
{
    return retain(handle, use, debugIdentifier, std::move(releaseDeviceResources), {});
}


DrawableUsageResult DrawableCache::retain(const DrawableResourceHandle& handle, DrawableUse use,
    const std::string& debugIdentifier, std::function<void()> releaseDeviceResources,
    std::function<DrawableDependencies()> dependencies)
try
{
    if (!_impl->active || _impl->busy)
    {
        return {{}, {Code::Busy, "Drawable cache is already updating a resource"}};
    }
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return {{}, {Code::ForeignResource, "Drawable handle is stale or belongs to another cache"}};
    }
    gpu::requireDebugName(debugIdentifier);
    switch (use)
    {
    case DrawableUse::SceneOwner:
    case DrawableUse::SceneInstance:
    case DrawableUse::History:
    case DrawableUse::Job:
    case DrawableUse::Frame:
        break;
    default:
        return {{}, {Code::InvalidArgument, "Unknown drawable usage category"}};
    }
    if (use == DrawableUse::SceneOwner && entry->usage().sceneOwners)
    {
        return {{}, {Code::InUse, "A drawable resource may have only one scene owner"}};
    }
    auto record = std::make_shared<detail::DrawableUsageRecord>();
    record->entry = entry;
    record->context = shared_from_this();
    record->use = use;
    record->debugIdentifier = debugIdentifier;
    record->revision = entry->revision;
    record->releaseDeviceResources = std::move(releaseDeviceResources);
    record->dependencies = std::move(dependencies);
    if (use == DrawableUse::History || use == DrawableUse::Job || use == DrawableUse::Frame)
    {
        record->realization = entry->resource;
        record->originals = entry->originals;
        record->originalTransform = entry->originalTransform;
    }
    std::erase_if(entry->uses, [](const auto& weak) { return weak.expired(); });
    entry->uses.push_back(record);
    DrawableUsageLease lease;
    lease._record = std::move(record);
    return {std::move(lease), {}};
}
catch (const std::exception& error)
{
    return {{}, {Code::ResourceFailed, error.what()}};
}
catch (...)
{
    return {{}, {Code::ResourceFailed, "Cannot retain drawable use"}};
}


DrawableCacheDiagnostic DrawableCache::canUnload(const DrawableResourceHandle& handle) const
{
    if (!_impl->active || _impl->busy)
    {
        return {Code::Busy, "Drawable cache is already updating a resource"};
    }
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return {Code::ForeignResource, "Drawable handle is stale or belongs to another cache"};
    }
    if (!entry->usage().unused())
    {
        return {Code::InUse, "Drawable is retained by a scene owner, instance, history entry, job or frame"};
    }
    return {};
}


DrawableCacheResult DrawableCache::unload(const DrawableResourceHandle& handle)
try
{
    const auto diagnostic = canUnload(handle);
    if (!diagnostic.ok())
    {
        return {{}, diagnostic};
    }
    const auto entry = ownedEntry(handle);
    Mutation mutation(_impl->busy);
    gpu::Device::ResourceUpdateScope boundary(*_impl->device);
    _impl->device->waitIdleForResourceUpdate();
    // Retired versions may still have unregistered read snapshots; release all
    // versions of this identity before allowing a later registration elsewhere.
    for (const auto& weak : _impl->realizations)
    {
        if (const auto resource = weak.lock(); resource && resource->_cacheEntry.lock() == entry)
        {
            releaseRealization(*resource);
        }
    }
    entry->resident = false;
    _impl->entries.erase(entry->identifier);
    ++_impl->generation;
    return {};
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot unload drawable; resident entry retained");
}


DrawableCacheResult DrawableCache::clear()
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is already updating a resource");
    }
    for (const auto& [id, entry] : _impl->entries)
    {
        if (!entry->usage().unused())
        {
            return failure(Code::InUse, "Cannot clear drawable cache while a resource is retained");
        }
    }
    Mutation mutation(_impl->busy);
    gpu::Device::ResourceUpdateScope boundary(*_impl->device);
    _impl->device->waitIdleForResourceUpdate();
    releaseResources();
    ++_impl->generation;
    return {};
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot clear drawable cache; resident entries retained");
}


DrawableCacheResult DrawableCache::setExportPath(const DrawableResourceHandle& handle, const std::filesystem::path& path)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is already updating a resource");
    }
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return failure(Code::ForeignResource, "Drawable handle is stale or belongs to another cache");
    }
    auto resultHandle = handleFor(entry);
    auto destination = path.empty() ? std::filesystem::path{} : normalizedPath(path);
    entry->exportPath.swap(destination);
    ++_impl->generation;
    return {std::move(resultHandle), {}};
}
catch (const std::exception& error)
{
    return failure(Code::InvalidArgument, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot update drawable export destination");
}


DrawableCacheResult DrawableCache::recordExportPublication(const DrawableResourceHandle& handle,
    const std::filesystem::path& path, const std::vector<glm::mat4>& transforms)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is not available for export bookkeeping");
    }
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return failure(Code::ForeignResource, "Cannot record export for an expired resource");
    }
    if (!entry->resource || transforms.size() != entry->resource->submeshesCount())
    {
        return failure(Code::InvalidArgument, "Published export basis must cover the resource submeshes");
    }
    for (const auto& transform : transforms)
    {
        for (int c = 0; c < 4; ++c)
        {
            for (int r = 0; r < 4; ++r)
            {
                if (!std::isfinite(transform[c][r]))
                {
                    return failure(Code::InvalidArgument, "Published export basis must be finite");
                }
            }
        }
        const auto determinant = glm::determinant(transform);
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f ||
            transform[0][3] != 0 || transform[1][3] != 0 || transform[2][3] != 0 || transform[3][3] != 1)
        {
            return failure(Code::InvalidArgument, "Published export basis must be invertible and affine");
        }
    }
    auto destination = normalizedPath(path);
    auto records = entry->publishedExports;
    records.erase(std::remove_if(records.begin(), records.end(), [&](const auto& value)
    {
        return value.path == destination;
    }), records.end());
    records.push_back({destination, transforms});
    auto resultHandle = handleFor(entry);
    entry->publishedExports.swap(records);
    entry->exportPath.swap(destination);
    ++_impl->generation;
    return {std::move(resultHandle), {}};
}
catch (const std::exception& error)
{
    return failure(Code::ResourceFailed, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot retain drawable export bookkeeping");
}


DrawableCacheResult DrawableCache::addLookupAlias(const DrawableResourceHandle& handle, const std::filesystem::path& bg2Path)
try
{
    if (!_impl->active || _impl->busy)
    {
        return failure(Code::Busy, "Drawable cache is already updating a resource");
    }
    const auto entry = ownedEntry(handle);
    if (!entry)
    {
        return failure(Code::ForeignResource, "Drawable handle is stale or belongs to another cache");
    }
    const auto path = normalizedPath(bg2Path);
    const auto existing = find(path);
    if (existing)
    {
        return existing.handle == handle ? existing : failure(Code::Collision, "Drawable source alias belongs to another resource");
    }
    if (existing.diagnostic.code != Code::NotFound)
    {
        return existing;
    }
    auto resultHandle = handleFor(entry);
    entry->aliases.push_back(path);
    ++_impl->generation;
    return {std::move(resultHandle), {}};
}
catch (const std::exception& error)
{
    return failure(Code::InvalidArgument, error.what());
}
catch (...)
{
    return failure(Code::ResourceFailed, "Cannot add drawable source alias");
}


void DrawableCache::releaseRealization(Drawable& resource) noexcept
{
    // Preserve owner-authored CPU appearance separately from immutable originals.
    try
    {
        for (size_t i = 0; i < resource._materials.size() && i < resource._attributes.size(); ++i)
        {
            if (resource._materials[i])
            {
                resource._attributes[i].material = resource._materials[i]->materialAttributes();
            }
        }
    }
    catch (...)
    {
        std::fprintf(stderr, "Cannot retain all owner material fields during drawable context shutdown.\n");
    }
    // Clear BLAS before the raster buffers they borrow, while the device is alive.
    resource._tracing.clear();
    resource._materials.clear();
    resource._drawMesh.reset();
    resource._loaded = false;
    resource._engine = nullptr;
    resource._cacheEntry.reset();
}


void DrawableCache::releaseResources() noexcept
{
    const auto wasBusy = std::exchange(_impl->busy, true);
    for (const auto& [id, entry] : _impl->entries)
    {
        for (const auto& weak : entry->uses)
        {
            if (const auto record = weak.lock(); record && record->releaseDeviceResources)
            {
                try
                {
                    record->releaseDeviceResources();
                }
                catch (...)
                {
                    std::fprintf(stderr, "Drawable cache: device-release callback failed for %s\n", record->debugIdentifier.c_str());
                }
                record->releaseDeviceResources = {};
            }
        }
    }
    for (const auto& weak : _impl->realizations)
    {
        if (const auto resource = weak.lock())
        {
            const auto entry = resource->_cacheEntry.lock();
            if (entry && entry->cache == this)
            {
                releaseRealization(*resource);
            }
        }
    }
    for (const auto& [id, entry] : _impl->entries)
    {
        entry->resident = false;
    }
    _impl->entries.clear();
    _impl->realizations.clear();
    _impl->busy = wasBusy;
}

}
