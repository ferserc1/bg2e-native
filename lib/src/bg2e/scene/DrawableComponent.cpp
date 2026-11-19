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

#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/utils/utils.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/drawable_cache.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <bg2e/gpu/Device.hpp>

#include <iostream>
#include <stdexcept>

namespace bg2e::scene {

DrawableComponent::DrawableComponent() : _ownerToken(std::make_shared<const int>(0))
{
}


DrawableComponent::DrawableComponent(std::shared_ptr<draw::Drawable> resource) : DrawableComponent()
{
    (void)setDrawable(std::move(resource));
}


DrawableComponent::DrawableComponent(draw::Drawable* resource) : DrawableComponent()
{
    // Never create a second owning shared_ptr for an already owned raw pointer.
    if (resource && !resource->_ownerClaim.expired())
    {
        (void)report({draw::DrawableCacheCode::InUse, "Drawable already has an owner; clone it or create an instance"});
        return;
    }
    if (resource && resource->cacheUsage().registered)
    {
        const auto handle = resource->resourceHandle();
        const auto cache = handle.context();
        if (cache)
        {
            (void)rebindResource(*cache, handle);
            return;
        }
    }
    (void)setDrawable(std::shared_ptr<draw::Drawable>(resource));
}


DrawableComponent::DrawableComponent(std::nullptr_t) : DrawableComponent()
{
}


DrawableComponent::DrawableComponent(const DrawableComponent& other) : DrawableComponent()
{
    *this = other;
}


DrawableComponent& DrawableComponent::operator=(const DrawableComponent& other)
{
    if (this == &other)
    {
        return *this;
    }
    try
    {
        if (_drawable && _drawable->engine() && other._drawable && other._drawable->engine() &&
            _drawable->engine() != other._drawable->engine())
        {
            (void)report({draw::DrawableCacheCode::CrossDevice, "Cannot copy an owner across active engine contexts"});
            return *this;
        }
        auto prepared = std::static_pointer_cast<DrawableComponent>(other.DrawableComponent::clone());
        if (!prepared)
        {
            (void)report({draw::DrawableCacheCode::ResourceFailed, "Cannot prepare an independent drawable owner copy"});
            return *this;
        }
        if (_drawable && _drawable->engine())
        {
            _drawable->engine()->device()->waitIdleForResourceUpdate();
        }
        adoptPreparedCopy(*prepared);
    }
    catch (const std::exception& error)
    {
        (void)report({draw::DrawableCacheCode::ResourceFailed, error.what()});
    }
    catch (...)
    {
        (void)report({draw::DrawableCacheCode::ResourceFailed, "Cannot copy drawable owner; previous state retained"});
    }
    return *this;
}


void DrawableComponent::adoptPreparedCopy(DrawableComponent& prepared)
{
    if (_drawable && _drawable->_ownerClaim.lock() == _ownerToken)
    {
        _drawable->_ownerClaim.reset();
    }
    // Membership and enable_shared_from_this state belong to this object, not
    // the source. Transfer the new reservation together with its realization.
    _ownerToken = std::move(prepared._ownerToken);
    _drawable = std::move(prepared._drawable);
    _ownerUse = std::move(prepared._ownerUse);
    _cacheContext = std::move(prepared._cacheContext);
    _handle = std::move(prepared._handle);
    _resourcePath = std::move(prepared._resourcePath);
    _registrationSourcePath = std::move(prepared._registrationSourcePath);
    _pendingExportPath = std::move(prepared._pendingExportPath);
    _copySourcePath = std::move(prepared._copySourcePath);
    _resourceName = std::move(prepared._resourceName);
    _independentCopy = prepared._independentCopy;
    _explicitlyCleared = prepared._explicitlyCleared;
    _copyAttempted = prepared._copyAttempted;
    _assignmentDiagnostic = std::move(prepared._assignmentDiagnostic);
    _resourceWarning.clear();
    setPriority(prepared.priority());
    setIgnoreSerialization(prepared.ignoreSerialization());
}


DrawableComponent::~DrawableComponent()
{
    if (_drawable && _drawable->_ownerClaim.lock() == _ownerToken)
    {
        _drawable->_ownerClaim.reset();
    }
}


draw::DrawableCacheDiagnostic DrawableComponent::report(draw::DrawableCacheDiagnostic diagnostic)
{
    _assignmentDiagnostic = std::move(diagnostic);
    if (!_assignmentDiagnostic.ok())
    {
        std::cerr << "Drawable owner: " << _assignmentDiagnostic.message << '\n';
    }
    return _assignmentDiagnostic;
}


const draw::DrawableCacheDiagnostic& DrawableComponent::assignmentDiagnostic() const noexcept
{
    return _assignmentDiagnostic;
}


const draw::DrawableResourceHandle& DrawableComponent::resourceHandle() const noexcept
{
    return _handle;
}


bool DrawableComponent::renderable(bool) const
{
    return !_explicitlyCleared;
}


std::shared_ptr<draw::Drawable> DrawableComponent::drawable()
{
    return _drawable;
}


std::shared_ptr<const draw::Drawable> DrawableComponent::drawable() const
{
    return _drawable;
}


draw::DrawableCacheDiagnostic DrawableComponent::setDrawable(std::shared_ptr<draw::Drawable> resource)
try
{
    if (!resource)
    {
        return clearDrawable();
    }
    // Overlays deliberately share tool realizations and are not authored owners.
    if (!isSceneGeometry())
    {
        if (_drawable && _drawable->engine() && resource->engine() && _drawable->engine() != resource->engine())
        {
            return report({draw::DrawableCacheCode::CrossDevice, "Overlay drawable belongs to another draw context"});
        }
        if (_drawable && _drawable->engine())
        {
            _drawable->engine()->device()->waitIdleForResourceUpdate();
        }
        _drawable = std::move(resource);
        _explicitlyCleared = false;
        return report({});
    }
    if (!_ownerToken)
    {
        _ownerToken = std::make_shared<const int>(0);
    }
    if (!resource->mesh() || !resource->submeshesCount())
    {
        return report({draw::DrawableCacheCode::InvalidArgument, "A drawable owner requires complete CPU geometry"});
    }
    const auto claim = resource->_ownerClaim.lock();
    if (claim && claim != _ownerToken)
    {
        return report({draw::DrawableCacheCode::InUse, "Drawable already belongs to another owner; use an instance or clone"});
    }
    if (_drawable == resource)
    {
        return report({});
    }
    if (_drawable && _drawable->engine() && resource->engine() && _drawable->engine() != resource->engine())
    {
        return report({draw::DrawableCacheCode::CrossDevice, "Clear the owner at its old engine boundary before changing devices"});
    }
    if (!_handle.valid() && resource->cacheUsage().registered)
    {
        const auto handle = resource->resourceHandle();
        const auto cache = handle.context();
        return cache ? rebindResource(*cache, handle) : report({draw::DrawableCacheCode::ForeignResource, "Resource context expired"});
    }
    const bool newBinding = !_handle.valid();
    auto origin = _registrationSourcePath;
    if (newBinding && origin.empty())
    {
        origin = resource->importSource();
    }
    const bool independent = _independentCopy || (newBinding && resource->isResourceClone());
    // Reserve ownership before any uploads/callbacks can observe the candidate.
    resource->_ownerClaim = _ownerToken;
    if (_handle.valid())
    {
        const auto replaced = _cacheContext->replace(_handle, resource);
        if (!replaced)
        {
            if (!claim)
            {
                resource->_ownerClaim.reset();
            }
            return report(replaced.diagnostic);
        }
    }
    else if (_drawable && _drawable->engine())
    {
        _drawable->engine()->device()->waitIdleForResourceUpdate();
    }
    if (_drawable && _drawable->_ownerClaim.lock() == _ownerToken)
    {
        _drawable->_ownerClaim.reset();
    }
    _drawable = std::move(resource);
    _registrationSourcePath = std::move(origin);
    _independentCopy = independent;
    _explicitlyCleared = false;
    return report({});
}
catch (const std::exception& error)
{
    if (resource && resource != _drawable && resource->_ownerClaim.lock() == _ownerToken)
    {
        resource->_ownerClaim.reset();
    }
    return report({draw::DrawableCacheCode::ResourceFailed, error.what()});
}
catch (...)
{
    if (resource && resource != _drawable && resource->_ownerClaim.lock() == _ownerToken)
    {
        resource->_ownerClaim.reset();
    }
    return report({draw::DrawableCacheCode::ResourceFailed, "Cannot assign drawable; previous owner state retained"});
}


draw::DrawableCacheDiagnostic DrawableComponent::clearDrawable()
try
{
    if (_drawable && _drawable->engine())
    {
        _drawable->engine()->device()->waitIdleForResourceUpdate();
    }
    if (_drawable && _drawable->_ownerClaim.lock() == _ownerToken)
    {
        _drawable->_ownerClaim.reset();
    }
    _drawable.reset();
    _explicitlyCleared = true;
    _ownerUse = {};
    _handle = {};
    _cacheContext.reset();
    return report({});
}
catch (const std::exception& error)
{
    return report({draw::DrawableCacheCode::ResourceFailed, error.what()});
}
catch (...)
{
    return report({draw::DrawableCacheCode::ResourceFailed, "Cannot release owner; previous binding retained"});
}


draw::DrawableCacheDiagnostic DrawableComponent::rebindResource(draw::DrawableCache& cache,
    const draw::DrawableResourceHandle& handle)
try
{
    if (_handle == handle && _ownerUse.valid())
    {
        return report({});
    }
    auto resource = std::const_pointer_cast<draw::Drawable>(cache.resolve(handle));
    if (!resource)
    {
        return report({draw::DrawableCacheCode::ForeignResource, "Cannot bind an expired or foreign drawable identity"});
    }
    if (_drawable && _drawable->engine() && _drawable->engine() != &cache.engine())
    {
        return report({draw::DrawableCacheCode::CrossDevice, "Cannot rebind an owner across active engine contexts"});
    }
    const auto claim = resource->_ownerClaim.lock();
    if (claim && claim != _ownerToken)
    {
        return report({draw::DrawableCacheCode::InUse, "Resource already has an exclusive owner"});
    }
    const auto info = cache.inspect(handle);
    if (!info)
    {
        return report({draw::DrawableCacheCode::ForeignResource, "Resource identity is no longer resident"});
    }
    const bool changingResource = _drawable != resource;
    auto reference = _resourcePath;
    if (changingResource)
    {
        reference = !info->exportPath.empty() ? info->exportPath :
            info->kind == draw::DrawableResourceKind::Bg2 ? info->sourcePath : std::filesystem::path{};
    }
    auto origin = changingResource || _registrationSourcePath.empty() ? info->sourcePath : _registrationSourcePath;
    auto pending = changingResource ? std::filesystem::path{} : _pendingExportPath;
    if (changingResource && !info->exportPath.empty() && info->exportPath != info->sourcePath)
    {
        pending = info->exportPath; // Planned destination is not publication proof.
    }
    auto preparedHandle = handle;
    auto context = cache.shared_from_this();
    auto use = cache.retain(handle, draw::DrawableUse::SceneOwner, "drawable-owner:" + handle.identifier());
    if (!use)
    {
        return report(use.diagnostic);
    }
    if (_drawable && _drawable->engine())
    {
        _drawable->engine()->device()->waitIdleForResourceUpdate();
    }
    if (_drawable && _drawable != resource && _drawable->_ownerClaim.lock() == _ownerToken)
    {
        _drawable->_ownerClaim.reset();
    }
    resource->_ownerClaim = _ownerToken;
    _drawable = std::move(resource);
    _cacheContext = std::move(context);
    _handle = std::move(preparedHandle);
    _ownerUse = std::move(use.lease);
    _resourcePath.swap(reference);
    _registrationSourcePath.swap(origin);
    _pendingExportPath.swap(pending);
    _explicitlyCleared = false;
    return report({});
}
catch (const std::exception& error)
{
    return report({draw::DrawableCacheCode::ResourceFailed, error.what()});
}
catch (...)
{
    return report({draw::DrawableCacheCode::ResourceFailed, "Cannot rebind owner; previous identity retained"});
}


void DrawableComponent::assignIndependentPath()
{
    if (_resourcePath.empty() || !_pendingExportPath.empty())
    {
        return;
    }
    if (_registrationSourcePath.empty())
    {
        _registrationSourcePath = _resourcePath;
    }
    _copySourcePath = _resourcePath;
    const auto stem = _registrationSourcePath.stem().string();
    const auto directory = _resourcePath.parent_path();
    // A unique suffix is also checked against the filesystem and planned cache
    // exports. Neither a collision nor a Save As destination may be overwritten.
    bool collision;
    do
    {
        _pendingExportPath = directory / (stem + "_" + utils::uniqueId() + ".bg2");
        std::error_code error;
        collision = std::filesystem::exists(_pendingExportPath, error);
        if (_cacheContext)
        {
            for (const auto& info : _cacheContext->entries())
            {
                collision = collision || info.exportPath == _pendingExportPath || info.sourcePath == _pendingExportPath;
            }
        }
    }
    while (collision);
    _resourcePath = _pendingExportPath;
    _copyAttempted = false;
}


void DrawableComponent::publishIndependentCopy() noexcept
try
{
    if (!_independentCopy || _pendingExportPath.empty() || _copyAttempted)
    {
        return;
    }
    _copyAttempted = true;
    const auto input = _copySourcePath.empty() ? sourcePath() : _copySourcePath;
    auto extension = input.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    if (extension != ".bg2" && extension != ".vwglb")
    {
        // Imported formats require model export, not a renamed byte copy.
        return;
    }
    if (input.parent_path() != _pendingExportPath.parent_path())
    {
        std::fprintf(stderr, "Independent model copy needs image rebasing; retaining pending destination for complete export.\n");
        return;
    }
    std::error_code error;
    if (!std::filesystem::copy_file(input, _pendingExportPath, std::filesystem::copy_options::none, error))
    {
        std::cerr << "Cannot publish independent drawable copy " << _pendingExportPath << ": " << error.message()
            << "; independent in-memory resource and pending destination retained.\n";
        return;
    }
    if (_handle.valid())
    {
        const auto alias = _cacheContext->addLookupAlias(_handle, _pendingExportPath);
        if (!alias)
        {
            std::cerr << "Cannot register independent drawable copy alias: " << alias.diagnostic.message << '\n';
            return;
        }
    }
    _pendingExportPath.clear();
}
catch (...)
{
    std::fprintf(stderr, "Cannot publish independent drawable copy; in-memory owner and pending destination retained.\n");
}


std::shared_ptr<Component> DrawableComponent::clone() const
{
    auto copy = std::make_shared<DrawableComponent>();
    copy->_resourcePath = _resourcePath;
    copy->_resourceName = _drawable ? std::optional<std::string>(_drawable->name()) : _resourceName;
    copy->_registrationSourcePath = _registrationSourcePath;
    if (copy->_registrationSourcePath.empty() && _handle.valid())
    {
        const auto info = _cacheContext->inspect(_handle);
        if (info)
        {
            copy->_registrationSourcePath = info->sourcePath;
        }
    }
    copy->_priority = _priority;
    copy->_ignoreSerialization = _ignoreSerialization;
    copy->_independentCopy = true;
    copy->_explicitlyCleared = _explicitlyCleared;
    copy->_cacheContext = _cacheContext;
    if (_drawable)
    {
        const auto assigned = copy->setDrawable(_drawable->clone());
        if (!assigned.ok())
        {
            return nullptr;
        }
        copy->assignIndependentPath();
        if (_drawable->engine())
        {
            copy->loadResources(*_drawable->engine());
        }
    }
    return copy;
}


std::filesystem::path DrawableComponent::sourcePath() const
{
    if (!_registrationSourcePath.empty())
    {
        return _registrationSourcePath;
    }
    if (_handle.valid())
    {
        const auto info = _cacheContext->inspect(_handle);
        if (info && !info->sourcePath.empty())
        {
            return info->sourcePath;
        }
    }
    return _resourcePath;
}


void DrawableComponent::setResourcePath(const std::filesystem::path& path)
{
    auto reference = path.empty() ? path : std::filesystem::absolute(path).lexically_normal();
    if (_handle.valid() && reference.extension() == ".bg2")
    {
        const auto planned = _cacheContext->setExportPath(_handle, reference);
        if (!planned)
        {
            (void)report(planned.diagnostic);
            return;
        }
    }
    _resourcePath.swap(reference);
    _explicitlyCleared = false;
    _pendingExportPath.clear();
    _copyAttempted = false;
    _assignmentDiagnostic = {};
}

void DrawableComponent::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath)
{
    if (_drawable)
    {
        throw std::logic_error("Clear DrawableComponent before reading a resource reference");
    }
    deserializeCommon(data);
    json::ObjectReader reader(data);
    _resourceName = reader.getString("resourceName");
    if (auto path = reader.getString("path"))
    {
        if (path->empty())
        {
            throw std::invalid_argument("Drawable source path cannot be empty");
        }
        setResourcePath(basePath / *path);
    }
    else if (auto name = reader.getString("name"))
    {
        if (name->empty())
        {
            throw std::invalid_argument("Drawable source name cannot be empty");
        }
        auto path = basePath / *name;
        path.replace_extension(".bg2");
        setResourcePath(path);
    }
    else
    {
        throw std::invalid_argument("Drawable representation requires a path or legacy name");
    }
    if (_resourcePath.empty())
    {
        throw std::invalid_argument("Drawable source path cannot be empty");
    }
}

std::shared_ptr<json::JsonNode> DrawableComponent::serializeReferences(const std::filesystem::path& basePath)
{
    if (_resourcePath.empty() || (!_pendingExportPath.empty() && _resourcePath == _pendingExportPath))
    {
        throw std::logic_error("Drawable needs a published source path for reference-only serialization; use a complete scene save");
    }
    auto result = Component::serialize(basePath);
    const auto base = basePath.empty() ? std::filesystem::current_path() : std::filesystem::absolute(basePath);
    const auto relative = _resourcePath.lexically_relative(base.lexically_normal());
    if (_drawable)
    {
        result->objectValue()["resourceName"] = json::JSON(_drawable->name());
    }
    else if (_resourceName)
    {
        result->objectValue()["resourceName"] = json::JSON(*_resourceName);
    }
    result->objectValue()["path"] = json::JSON((relative.empty() ? _resourcePath : relative).generic_string());
    return result;
}

std::shared_ptr<json::JsonNode> DrawableComponent::serializeExportReferences(const std::filesystem::path& basePath,
    const std::filesystem::path& modelPath)
{
    if (modelPath.empty())
    {
        throw std::invalid_argument("Owner export requires a prepared model destination");
    }
    auto reference = std::filesystem::absolute(modelPath).lexically_normal();
    std::filesystem::path pending;
    _resourcePath.swap(reference);
    _pendingExportPath.swap(pending);
    try
    {
        auto result = serializeReferences(basePath);
        _pendingExportPath.swap(pending);
        _resourcePath.swap(reference);
        return result;
    }
    catch (...)
    {
        _pendingExportPath.swap(pending);
        _resourcePath.swap(reference);
        throw;
    }
}


draw::DrawableCacheDiagnostic DrawableComponent::registerResource(draw::DrawableCache& cache)
{
    if (_handle.valid())
    {
        const auto refreshed = _cacheContext->refresh(_handle);
        return report(refreshed.diagnostic);
    }
    if (!_drawable)
    {
        return report({draw::DrawableCacheCode::InvalidArgument, "Drawable owner has no geometry to register"});
    }
    if (_drawable->cacheUsage().registered)
    {
        const auto handle = _drawable->resourceHandle();
        const auto context = handle.context();
        return context ? rebindResource(*context, handle) : report({draw::DrawableCacheCode::ForeignResource, "Resource cache expired"});
    }
    const auto displayName = !_drawable->name().empty() ? _drawable->name() : ownerNode() && !ownerNode()->name().empty() ?
        ownerNode()->name() : "authored-model";
    draw::DrawableCacheResult registered;
    const auto canonicalSource = _resourcePath.empty() ? sourcePath() : _resourcePath;
    auto extension = canonicalSource.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    if (!_independentCopy && (extension == ".bg2" || extension == ".vwglb"))
    {
        const auto found = cache.find(canonicalSource);
        if (found)
        {
            // This owner already has separate geometry. Do not overwrite the
            // library resource or silently turn a legacy duplicate into sharing.
            _independentCopy = true;
            std::cerr << "Ambiguous duplicate owner path " << canonicalSource
                << "; instances keep the first resource and this owner receives an independent copy.\n";
            assignIndependentPath();
            registered = cache.registerImported(_drawable, _registrationSourcePath.empty() ? _resourcePath : _registrationSourcePath, displayName);
        }
        else
        {
            registered = cache.load(canonicalSource, [resource = _drawable](const auto&, auto&)
            {
                return resource;
            });
        }
    }
    else if (!_resourcePath.empty() || !_registrationSourcePath.empty())
    {
        registered = cache.registerImported(_drawable, _registrationSourcePath.empty() ? _resourcePath : _registrationSourcePath, displayName);
    }
    else
    {
        registered = cache.registerProcedural(_drawable, displayName);
    }
    if (!registered)
    {
        return report(registered.diagnostic);
    }
    const auto bound = rebindResource(cache, registered.handle);
    if (!bound.ok())
    {
        return bound;
    }
    if (!_resourcePath.empty() && _resourcePath.extension() == ".bg2")
    {
        const auto destination = cache.setExportPath(_handle, _resourcePath);
        if (!destination)
        {
            return report(destination.diagnostic);
        }
    }
    publishIndependentCopy();
    return report({});
}


void DrawableComponent::loadResources(draw::Engine& engine, SceneLoadProgress* progress)
{
    if (!isSceneGeometry())
    {
        if (_drawable && _drawable->engine() && _drawable->engine() != &engine)
        {
            throw std::logic_error("Overlay drawable belongs to another draw engine");
        }
        if (_drawable && !_drawable->isLoaded())
        {
            _drawable->load(&engine);
        }
        return;
    }
    if (!_drawable && !_assignmentDiagnostic.ok())
    {
        throw std::runtime_error(_assignmentDiagnostic.message);
    }
    const bool firstBinding = !_handle.valid();
    auto context = _handle.valid() ? _cacheContext : ownerNode() ? ownerNode()->drawableCacheContext() : _cacheContext;
    if (!context || !context->active())
    {
        context = engine.drawableCache().shared_from_this();
    }
    if (&context->engine() != &engine || (_drawable && _drawable->engine() && _drawable->engine() != &engine))
    {
        throw std::logic_error("Drawable owner belongs to another draw engine");
    }
    if (!_drawable)
    {
        auto onTextureLoaded = [progress]()
        {
            if (progress)
            {
                progress->total = std::max(progress->total, progress->loaded + 1);
                if (progress->callback)
                {
                    progress->callback("texture", progress->loaded, progress->total);
                }
                ++progress->loaded;
            }
        };
        if (_resourcePath.extension() == ".bg2" || _resourcePath.extension() == ".vwglb")
        {
            if (_independentCopy)
            {
                const auto input = sourcePath();
                const auto prepared = input.extension() == ".obj" ? db::loadDrawableObj(input, engine, onTextureLoaded) :
                    db::loadDrawableBg2(input, engine, onTextureLoaded);
                const auto assigned = setDrawable(prepared);
                if (!assigned.ok())
                {
                    throw std::runtime_error(assigned.message);
                }
                assignIndependentPath();
            }
            else
            {
                const auto loaded = db::loadCachedDrawableBg2(_resourcePath, *context, onTextureLoaded);
                if (!loaded)
                {
                    throw std::runtime_error(loaded.diagnostic.message);
                }
                const auto source = context->resolve(loaded.handle);
                if (context->queryUsage(loaded.handle).sceneOwners)
                {
                    _independentCopy = true;
                    std::cerr << "Ambiguous duplicate owner reference; instances retain the first resource at " << _resourcePath
                        << "; creating an independent owner and destination.\n";
                    const auto assigned = setDrawable(source->clone());
                    if (!assigned.ok())
                    {
                        throw std::runtime_error(assigned.message);
                    }
                    assignIndependentPath();
                }
                else
                {
                    const auto bound = rebindResource(*context, loaded.handle);
                    if (!bound.ok())
                    {
                        throw std::runtime_error(bound.message);
                    }
                }
            }
        }
        else if (_resourcePath.extension() == ".obj")
        {
            const auto assigned = setDrawable(db::loadDrawableObj(_resourcePath, engine, onTextureLoaded));
            if (!assigned.ok())
            {
                throw std::runtime_error(assigned.message);
            }
        }
        else
        {
            throw std::invalid_argument("Unsupported draw resource: " + _resourcePath.string());
        }
    }
    if (!_drawable->isLoaded())
    {
        _drawable->load(&engine);
    }
    if (firstBinding && _resourceName)
    {
        _drawable->setName(*_resourceName);
    }
    const auto registered = registerResource(*context);
    if (!registered.ok())
    {
        throw std::runtime_error(registered.message);
    }
}


bool DrawableComponent::prepareResources(Node* root, draw::Engine& engine) noexcept
{
    if (!root)
    {
        return false;
    }
    bool changed = false;
    if (!root->drawableCacheContext() || !root->drawableCacheContext()->active())
    {
        try
        {
            root->setDrawableCacheContext(engine.createDrawableCache());
        }
        catch (...)
        {
            return false;
        }
    }
    for (const auto& component : root->orderedComponents())
    {
        auto* owner = dynamic_cast<DrawableComponent*>(component.get());
        if (!owner || !owner->isSceneGeometry() || owner->_explicitlyCleared)
        {
            continue;
        }
        try
        {
            const auto previous = owner->_handle.geometryRevision();
            owner->loadResources(engine);
            changed = owner->_handle.geometryRevision() != previous || changed;
            owner->_resourceWarning.clear();
        }
        catch (const std::exception& error)
        {
            try
            {
                if (owner->_resourceWarning != error.what())
                {
                    std::cerr << "Drawable preparation: " << error.what() << '\n';
                    owner->_resourceWarning = error.what();
                }
            }
            catch (...)
            {
                std::fprintf(stderr, "Cannot report drawable preparation failure.\n");
            }
        }
        catch (...)
        {
            std::fprintf(stderr, "Drawable preparation failed; previous owner state retained.\n");
        }
    }
    for (const auto& child : root->children())
    {
        changed = prepareResources(child.get(), engine) || changed;
    }
    return changed;
}


std::shared_ptr<draw::DrawableUsageLease> DrawableComponent::retainFrameResources() const
{
    if (!_handle.valid())
    {
        return nullptr;
    }
    draw::DrawableDependencies dependencies;
    for (uint32_t sub = 0; sub < _drawable->submeshesCount(); ++sub)
    {
        draw::appendMaterialDependencies(_drawable->materialDefinition(sub), dependencies);
        draw::appendMaterialDependencies(_drawable->material(sub), dependencies);
    }
    auto retained = _cacheContext->retain(_handle, draw::DrawableUse::Frame, "owner-frame:" + _handle.identifier(), {},
        [dependencies]()
        {
            return dependencies;
        });
    if (!retained)
    {
        throw std::runtime_error(retained.diagnostic.message);
    }
    return std::make_shared<draw::DrawableUsageLease>(std::move(retained.lease));
}


void DrawableComponent::collectDependencyIssues(const ComponentValidationContext&, std::vector<ComponentIssue>& issues) const
{
    if (!isSceneGeometry() || _explicitlyCleared)
    {
        return;
    }
    if (!_assignmentDiagnostic.ok())
    {
        issues.push_back({"drawable.assignment", ComponentIssueSeverity::Warning, _assignmentDiagnostic.message,
            "The previous resource or authored source reference was retained.", {}});
    }
    if (!_resourceWarning.empty())
    {
        issues.push_back({"drawable.resource", ComponentIssueSeverity::Warning, _resourceWarning,
            "Restore the source files or assign a valid model; other scene resources remain available.", {}});
    }
    if (hasPendingExport())
    {
        issues.push_back({"drawable.pending-copy", ComponentIssueSeverity::Warning,
            "The independent model destination has not been completely published.",
            "The independent in-memory model is retained. Save the complete scene before closing it.", {}});
    }
    else if (!_resourcePath.empty() && !resourcesLoaded())
    {
        issues.push_back({"drawable.unresolved", ComponentIssueSeverity::Warning,
            "Model resources are unavailable: " + _resourcePath.string(),
            "The source reference is retained; restore the model and its dependencies.", {}});
    }
}


std::shared_ptr<json::JsonNode> DrawableComponent::serialize(const std::filesystem::path& basePath)
{
    return serializeReferences(basePath);
}

BG2E_SCENE_REGISTER_COMPONENT(DrawableComponent);

}
