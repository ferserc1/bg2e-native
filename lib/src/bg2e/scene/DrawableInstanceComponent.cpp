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

#include <bg2e/scene/DrawableInstanceComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/draw/ImageLoadContext.hpp>
#include <bg2e/draw/ShaderData.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/base/Log.hpp>
#include <bg2e/db/drawable_cache.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/utils/MaterialSerializer.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace bg2e::scene {
namespace {

bool validTransform(const glm::mat4& value)
{
    for (int c = 0; c < 4; ++c)
    {
        for (int r = 0; r < 4; ++r)
        {
            if (!std::isfinite(value[c][r]))
            {
                return false;
            }
        }
    }
    const auto determinant = glm::determinant(value);
    return value[0][3] == 0 && value[1][3] == 0 && value[2][3] == 0 && value[3][3] == 1 &&
        std::isfinite(determinant) && std::abs(determinant) >= 1e-12f;
}


using Material = base::MaterialAttributes;
using TextureGetter = std::shared_ptr<base::Texture> (Material::*)() const;
using TextureSetter = void (Material::*)(std::shared_ptr<base::Texture>);
const char* TextureKeys[]{"albedoTexture", "metalnessTexture", "roughnessTexture", "normalTexture",
    "ambientOcclussion", "lightEmissionTexture"};
const TextureGetter TextureGetters[]{&Material::albedoTexture, &Material::metalnessTexture, &Material::roughnessTexture,
    &Material::normalTexture, &Material::aoTexture, &Material::lightEmissionTexture};
const TextureSetter TextureSetters[]{&Material::setAlbedoTexture, &Material::setMetalnessTexture, &Material::setRoughnessTexture,
    &Material::setNormalTexture, &Material::setAoTexture, &Material::setLightEmissionTexture};


std::shared_ptr<json::JsonNode> materialJson(Material attributes, const std::filesystem::path& base)
{
    utils::MaterialSerializer serializer;
    std::vector<std::shared_ptr<base::Texture>> textures;
    auto result = json::JsonParser(serializer.serializeMaterial(attributes, textures, false)).parse();
    auto& object = result->objectValue();
    for (size_t slot = 0; slot < 6; ++slot)
    {
        const auto texture = (attributes.*TextureGetters[slot])();
        if (!texture)
        {
            object[TextureKeys[slot]] = std::make_shared<json::JsonNode>();
        }
        else
        {
            if (texture->ignoreSerialization() || texture->type() != base::Texture::TypeFilesystem || texture->imageFilePath().empty())
            {
                throw std::logic_error("Instance material has an unexported texture; use a complete scene save");
            }
            const auto path = std::filesystem::absolute(texture->imageFilePath()).lexically_normal();
            const auto relative = path.lexically_relative(base);
            object[TextureKeys[slot]] = json::JSON((relative.empty() ? path : relative).generic_string());
        }
    }
    // These controls remain authored even when the corresponding texture is null.
    object["albedoScale"] = json::JSON(attributes.albedoScale());
    object["albedoUV"] = json::JSON(attributes.albedoUVSet());
    object["metalnessScale"] = json::JSON(attributes.metalnessScale());
    object["metalnessUV"] = json::JSON(attributes.metalnessUVSet());
    object["metalnessChannel"] = json::JSON(attributes.metalnessChannel());
    object["roughnessScale"] = json::JSON(attributes.roughnessScale());
    object["roughnessUV"] = json::JSON(attributes.roughnessUVSet());
    object["roughnessChannel"] = json::JSON(attributes.roughnessChannel());
    object["normalScale"] = json::JSON(attributes.normalScale());
    object["normalUV"] = json::JSON(attributes.normalUVSet());
    object["ambientOcclussionScale"] = json::JSON(attributes.aoScale());
    object["ambientOcclussionUV"] = json::JSON(attributes.aoUVSet());
    object["ambientOcclussionChannel"] = json::JSON(attributes.aoChannel());
    return result;
}


void relocateMaterials(const std::shared_ptr<json::JsonNode>& data, const std::filesystem::path& from,
    const std::filesystem::path& to)
{
    const auto submeshes = json::ObjectReader(data).getArray("submeshes");
    if (!submeshes)
    {
        return;
    }
    for (size_t i = 0; i < submeshes->size(); ++i)
    {
        const auto submesh = submeshes->getObject(i);
        const auto material = submesh ? submesh->getObject("material") : std::nullopt;
        if (!material)
        {
            continue;
        }
        for (const auto* key : TextureKeys)
        {
            if (const auto value = material->getString(key); value && !value->empty())
            {
                const auto absolute = std::filesystem::absolute(from / *value).lexically_normal();
                const auto relative = absolute.lexically_relative(to);
                material->node()->objectValue()[key] = json::JSON((relative.empty() ? absolute : relative).generic_string());
            }
        }
    }
}

}

namespace {
struct MaterialStamp {
    uint64_t revision = 0;
    const base::Texture* textures[6]{};
    uint64_t textureRevisions[6]{};

    static MaterialStamp capture(const base::MaterialAttributes& attributes) noexcept
    {
        MaterialStamp result;
        result.revision = attributes.revision();
        for (size_t i = 0; i < 6; ++i)
        {
            const auto texture = (attributes.*TextureGetters[i])();
            result.textures[i] = texture.get();
            result.textureRevisions[i] = texture ? texture->revision() : 0;
        }
        return result;
    }

    bool operator==(const MaterialStamp&) const = default;
};
}

struct DrawableInstanceComponent::State {
    std::vector<draw::Drawable::SubmeshAttributes> submeshes;
    std::vector<std::shared_ptr<draw::MaterialBase>> materials;
    std::string preparedKey;
    std::vector<MaterialStamp> stamps;
    bool deviceAvailable = false;
    std::vector<bool> materialEdits;

    draw::DrawableDependencies dependencies() const
    {
        draw::DrawableDependencies result;
        for (const auto& submesh : submeshes)
        {
            draw::appendMaterialDependencies(submesh.material, result);
        }
        for (const auto& material : materials)
        {
            if (material)
            {
                draw::appendMaterialDependencies(material->materialAttributes(), result);
            }
        }
        return result;
    }
};


DrawableInstanceComponent::DrawableInstanceComponent()
    : _state(std::make_shared<State>()), _debugIdentity(base::Log::nextDiagnosticIdentity())
{
}


DrawableInstanceComponent::~DrawableInstanceComponent() = default;


void DrawableInstanceComponent::warn(const char* message) const noexcept
{
    std::fprintf(stderr, "Drawable instance %llu '%s': %s\n", static_cast<unsigned long long>(_debugIdentity),
        _handle.identifier().c_str(), message);
}


draw::DrawableCacheDiagnostic DrawableInstanceComponent::setResource(draw::DrawableCache& cache,
    const draw::DrawableResourceHandle& handle)
{
    return bindResource(cache, handle, false);
}


draw::DrawableCacheDiagnostic DrawableInstanceComponent::bindResource(draw::DrawableCache& cache,
    const draw::DrawableResourceHandle& handle, bool restoreAppearance)
try
{
    if (_cache == &cache && _handle == handle && _use.valid())
    {
        return {};
    }
    const auto info = cache.inspect(handle);
    if (!info || !info->originalSubmeshes || !cache.resolve(handle))
    {
        return {draw::DrawableCacheCode::ForeignResource, "Instance needs a resident resource from the selected engine cache"};
    }
    // A component cannot silently migrate its already realized material context.
    if (_handle.valid() && _cache != &cache)
    {
        return {draw::DrawableCacheCode::CrossDevice, "Clear the instance at its old engine boundary before binding another cache"};
    }
    DrawableInstanceComponent candidate;
    candidate._handle = handle;
    candidate._cache = &cache;
    candidate._transform = info->originalTransform;
    candidate._tracing = cache.resolve(handle)->rayTracingEnabled();
    candidate._state->submeshes = *info->originalSubmeshes;
    for (auto& submesh : candidate._state->submeshes)
    {
        submesh.material = draw::cloneMaterialAttributes(submesh.material);
    }
    const std::weak_ptr<State> weak = candidate._state;
    auto use = cache.retain(handle, draw::DrawableUse::SceneInstance,
        "drawable-instance:" + handle.identifier() + ":instance-" + std::to_string(_debugIdentity), [weak]() noexcept
    {
        if (const auto state = weak.lock())
        {
            state->materials.clear();
            state->deviceAvailable = false;
        }
    }, [weak]()
    {
        const auto state = weak.lock();
        return state ? state->dependencies() : draw::DrawableDependencies{};
    });
    if (!use)
    {
        return use.diagnostic;
    }
    candidate._use = std::move(use.lease);
    if (restoreAppearance && _pendingAppearance)
    {
        candidate.overlayAppearance(_pendingAppearance, _appearanceBase);
        // A resident resource can still use the pre-export local geometry. The
        // incoming file reference is expressed in the published file basis.
        for (const auto& published : info->publishedExports)
        {
            if (published.path == _resourcePath)
            {
                for (size_t sub = 0; sub < std::min(candidate._state->submeshes.size(), published.submeshTransforms.size()); ++sub)
                {
                    candidate._state->submeshes[sub].transform *= published.submeshTransforms[sub];
                }
                break;
            }
        }
    }
    const bool published = std::find(info->lookupAliases.begin(), info->lookupAliases.end(), info->exportPath) != info->lookupAliases.end() ||
        std::any_of(info->publishedExports.begin(), info->publishedExports.end(), [&](const auto& value)
        {
            return value.path == info->exportPath;
        });
    auto path = published && !info->exportPath.empty() ? info->exportPath :
        info->kind == draw::DrawableResourceKind::Bg2 ? info->sourcePath : std::filesystem::path{};
    std::vector<glm::mat4> referenceBasis;
    for (const auto& value : info->publishedExports)
    {
        if (value.path == path)
        {
            referenceBasis = value.submeshTransforms;
            break;
        }
    }
    if (restoreAppearance && path.empty())
    {
        path = _resourcePath;
    }
    candidate.prepareResources(cache.engine());
    if (!candidate._ready && !restoreAppearance)
    {
        return {draw::DrawableCacheCode::ResourceFailed, "Instance material preparation failed; previous binding retained"};
    }
    if (_handle.valid())
    {
        _cache->engine().device()->waitIdleForResourceUpdate();
    }
    _state.swap(candidate._state);
    std::swap(_handle, candidate._handle);
    std::swap(_use, candidate._use);
    std::swap(_cache, candidate._cache);
    std::swap(_transform, candidate._transform);
    std::swap(_tracing, candidate._tracing);
    std::swap(_revision, candidate._revision);
    _ready = candidate._ready;
    _retry = false;
    _failureKey = std::move(candidate._failureKey);
    _resourcePath.swap(path);
    _exportBasis.swap(referenceBasis);
    _pendingAppearance.reset();
    _bindingFailure.clear();
    return {};
}
catch (const std::exception& error)
{
    return {draw::DrawableCacheCode::ResourceFailed, error.what()};
}
catch (...)
{
    return {draw::DrawableCacheCode::ResourceFailed, "Cannot bind instance; previous binding retained"};
}


const draw::DrawableResourceHandle& DrawableInstanceComponent::resourceHandle() const noexcept
{
    return _handle;
}


draw::DrawableCacheDiagnostic DrawableInstanceComponent::clearResource()
{
    const auto released = releaseResources();
    if (!released.ok())
    {
        return released;
    }
    _use = {};
    _handle = {};
    _cache = nullptr;
    _revision = 0;
    _ready = false;
    _resourcePath.clear();
    _exportBasis.clear();
    _pendingAppearance.reset();
    _bindingFailure.clear();
    return {};
}


draw::DrawableCacheDiagnostic DrawableInstanceComponent::releaseResources()
try
{
    if (_handle.valid())
    {
        _cache->engine().device()->waitIdleForResourceUpdate();
    }
    _state->materials.clear();
    _state->deviceAvailable = false;
    _ready = false;
    _retry = true;
    return {};
}
catch (const std::exception& error)
{
    warn(error.what());
    return {draw::DrawableCacheCode::ResourceFailed, error.what()};
}
catch (...)
{
    warn("Cannot synchronize instance resource release; previous resources retained.");
    return {draw::DrawableCacheCode::ResourceFailed, "Cannot release instance resources safely"};
}


std::string DrawableInstanceComponent::preparationKey() const
{
    std::ostringstream key;
    key << _handle.geometryRevision() << ':' << _state->submeshes.size();
    using Attributes = base::MaterialAttributes;
    using Getter = std::shared_ptr<base::Texture> (Attributes::*)() const;
    const Getter getters[]{&Attributes::albedoTexture, &Attributes::metalnessTexture, &Attributes::roughnessTexture,
        &Attributes::normalTexture, &Attributes::aoTexture, &Attributes::lightEmissionTexture};
    for (const auto& submesh : _state->submeshes)
    {
        key << ':' << submesh.material.revision();
        for (const auto getter : getters)
        {
            const auto texture = (submesh.material.*getter)();
            key << ':' << texture.get() << ':' << (texture ? texture->revision() : 0);
        }
    }
    return key.str();
}


bool DrawableInstanceComponent::needsPreparation() const
{
    if (!_handle.valid())
    {
        return false;
    }
    if (!_retry && _state->deviceAvailable && _revision == _handle.geometryRevision() &&
        _state->stamps.size() == _state->submeshes.size())
    {
        bool same = true;
        for (size_t i = 0; i < _state->stamps.size(); ++i)
        {
            same = same && _state->stamps[i] == MaterialStamp::capture(_state->submeshes[i].material);
        }
        if (same)
        {
            return false;
        }
    }
    const auto key = preparationKey();
    return _retry || ((!_state->deviceAvailable || _state->preparedKey != key) && key != _failureKey);
}


bool DrawableInstanceComponent::prepareResources(draw::Engine& engine) noexcept
{
    if (!_handle.valid())
    {
        loadResources(engine);
        return _handle.valid();
    }
    try
    {
        if (&engine != &_cache->engine())
        {
            warn("Cannot prepare materials on another engine; previous binding retained.");
            return false;
        }
        if (!needsPreparation())
        {
            return false;
        }
        const auto key = preparationKey();
        if (!_retry && key == _failureKey)
        {
            return false;
        }
        _retry = false;
        const auto info = _cache->inspect(_handle);
        const auto resource = renderResource();
        if (!info || !info->originalSubmeshes || !resource || !resource->isLoaded())
        {
            throw std::runtime_error("Shared geometry is not available");
        }
        const auto count = info->originalSubmeshes->size();
        // Geometry-only revisions do not invalidate independent material objects
        // or the weak material identities retained by existing undo commands.
        bool unchangedAppearance = _ready && _state->deviceAvailable && count == _state->submeshes.size() &&
            count == _state->materials.size() && count == _state->stamps.size();
        for (size_t i = 0; unchangedAppearance && i < count; ++i)
        {
            unchangedAppearance = _state->stamps[i] == MaterialStamp::capture(_state->submeshes[i].material);
        }
        if (unchangedAppearance)
        {
            _revision = info->geometryRevision;
            _failureKey.clear();
            return true;
        }
        auto submeshes = _state->submeshes;
        if (submeshes.size() != count)
        {
            warn("Shared topology changed; submesh state reconciled by index (excess truncated, missing defaults appended).");
            if (submeshes.size() > count)
            {
                submeshes.resize(count);
            }
            while (submeshes.size() < count)
            {
                auto value = info->originalSubmeshes->at(submeshes.size());
                value.material = draw::cloneMaterialAttributes(value.material);
                submeshes.push_back(std::move(value));
            }
        }
        // Reconcile authored CPU state independently of material realization.
        _state->submeshes.swap(submeshes);
        gpu::Device::ResourceUpdateScope boundary(*engine.device());
        draw::ImageLoadContext images(&engine);
        std::vector<std::shared_ptr<draw::MaterialBase>> materials;
        materials.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            auto material = i < _state->materials.size() ? _state->materials[i]->snapshot() :
                std::make_shared<draw::MaterialBase>(&engine);
            material->setMaterialAttributes(_state->submeshes[i].material);
            material->updateTextures(images);
            (void)draw::packMaterial(*material);
            materials.push_back(std::move(material));
        }
        auto preparedKey = preparationKey();
        std::vector<MaterialStamp> stamps;
        stamps.reserve(count);
        for (const auto& value : _state->submeshes)
        {
            stamps.push_back(MaterialStamp::capture(value.material));
        }
        engine.device()->waitIdleForResourceUpdate();
        _state->materials.swap(materials);
        _state->preparedKey.swap(preparedKey);
        _state->stamps.swap(stamps);
        _state->deviceAvailable = true;
        _revision = info->geometryRevision;
        _ready = true;
        _failureKey.clear();
        return true;
    }
    catch (const std::exception& error)
    {
        warn(error.what());
    }
    catch (...)
    {
        warn("Cannot prepare instance material resources; authored state retained.");
    }
    _ready = false;
    try
    {
        _failureKey = preparationKey();
    }
    catch (...)
    {
    }
    return true;
}


bool DrawableInstanceComponent::prepareResources(Node* root, draw::Engine& engine) noexcept
try
{
    if (!root)
    {
        return false;
    }
    bool changed = false;
    if (auto* instance = root->drawableInstance())
    {
        changed = instance->prepareResources(engine);
    }
    for (const auto& child : root->children())
    {
        changed = prepareResources(child.get(), engine) || changed;
    }
    return changed;
}
catch (...)
{
    std::fprintf(stderr, "Cannot traverse drawable instance preparation; deferred until the next boundary.\n");
    return false;
}


void DrawableInstanceComponent::retryResources() noexcept
{
    _retry = true;
    _failureKey.clear();
    _bindingFailure.clear();
}


void DrawableInstanceComponent::reportRenderFailure() noexcept
{
    if (_ready)
    {
        warn("Invalid instance render state; instance omitted until source/material change or retry.");
    }
    _ready = false;
    _retry = false;
    try
    {
        _failureKey = preparationKey();
    }
    catch (...)
    {
    }
}


std::shared_ptr<const draw::Drawable> DrawableInstanceComponent::drawable() const
{
    return nullptr;
}


std::shared_ptr<const draw::Drawable> DrawableInstanceComponent::renderResource() const
{
    return _handle.valid() && _cache ? _cache->resolve(_handle) : nullptr;
}


std::shared_ptr<draw::DrawableUsageLease> DrawableInstanceComponent::retainFrameResources() const
{
    if (!_handle.valid())
    {
        throw std::logic_error("Instance resource is no longer resident");
    }
    const auto dependencies = _state->dependencies();
    auto retained = _cache->retain(_handle, draw::DrawableUse::Frame,
        "instance-frame:" + _handle.identifier() + ":instance-" + std::to_string(_debugIdentity), {},
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


bool DrawableInstanceComponent::renderable(bool) const
{
    return _ready && _use.valid() && _state->deviceAvailable && _revision == _handle.geometryRevision();
}


bool DrawableInstanceComponent::resourcesLoaded() const
{
    return renderable() && DrawableRenderComponent::resourcesLoaded();
}


uint32_t DrawableInstanceComponent::retainedMaterialCount() const noexcept
{
    return static_cast<uint32_t>(_state->materials.size());
}


uint32_t DrawableInstanceComponent::submeshesCount() const
{
    return static_cast<uint32_t>(_state->submeshes.size());
}


glm::mat4 DrawableInstanceComponent::transform() const
{
    return _transform;
}


void DrawableInstanceComponent::setTransform(const glm::mat4& value)
{
    if (!validTransform(value))
    {
        warn("Invalid instance transform; previous transform retained.");
        return;
    }
    _transform = value;
    rememberField("transform", json::JSON(value));
}


glm::mat4 DrawableInstanceComponent::submeshTransform(uint32_t index) const
{
    return _transform * localSubmeshTransform(index);
}


glm::mat4 DrawableInstanceComponent::localSubmeshTransform(uint32_t index) const
{
    return _state->submeshes.at(index).transform;
}


void DrawableInstanceComponent::setSubmeshTransform(const glm::mat4& value, uint32_t index)
{
    if (!validTransform(value))
    {
        warn("Invalid submesh transform; previous transform retained.");
        return;
    }
    _state->submeshes.at(index).transform = value;
    rememberSubmesh(index, "transform", json::JSON(value));
}


const std::string& DrawableInstanceComponent::submeshName(uint32_t index) const
{
    return _state->submeshes.at(index).name;
}


const std::string& DrawableInstanceComponent::submeshGroupName(uint32_t index) const
{
    return _state->submeshes.at(index).groupName;
}


bool DrawableInstanceComponent::submeshVisibility(uint32_t index) const
{
    return _state->submeshes.at(index).visible;
}


void DrawableInstanceComponent::setSubmeshName(const std::string& value, uint32_t index)
{
    _state->submeshes.at(index).name = value;
    rememberSubmesh(index, "name", json::JSON(value));
}


void DrawableInstanceComponent::setSubmeshGroupName(const std::string& value, uint32_t index)
{
    _state->submeshes.at(index).groupName = value;
    rememberSubmesh(index, "groupName", json::JSON(value));
}


void DrawableInstanceComponent::setSubmeshVisibility(bool value, uint32_t index)
{
    _state->submeshes.at(index).visible = value;
    rememberSubmesh(index, "visible", json::JSON(value));
}


bool DrawableInstanceComponent::rayTracingEnabled() const
{
    return _tracing && DrawableRenderComponent::rayTracingEnabled();
}


void DrawableInstanceComponent::setRayTracingEnabled(bool value)
{
    _tracing = value;
    rememberField("rayTracingEnabled", json::JSON(value));
}


base::MaterialAttributes& DrawableInstanceComponent::material(uint32_t index)
{
    auto& value = _state->submeshes.at(index).material;
    if (_pendingAppearance)
    {
        _state->materialEdits.resize(_state->submeshes.size());
        _state->materialEdits.at(index) = true;
    }
    return value;
}


const base::MaterialAttributes& DrawableInstanceComponent::effectiveMaterial(uint32_t index) const
{
    return _state->submeshes.at(index).material;
}


void DrawableInstanceComponent::setMaterial(const base::MaterialAttributes& value, uint32_t index)
try
{
    auto copy = draw::cloneMaterialAttributes(value);
    auto serialized = _pendingAppearance ? materialJson(copy, _appearanceBase) : nullptr;
    rememberSubmesh(index, "material", std::move(serialized));
    _state->submeshes.at(index).material = std::move(copy);
}
catch (...)
{
    warn("Cannot replace instance material; previous authored material retained.");
}


void DrawableInstanceComponent::acknowledgeMaterialEdit(uint32_t index) noexcept
{
    if (_ready && _state->deviceAvailable && _revision == _handle.geometryRevision() &&
        index < _state->stamps.size() && index < _state->materials.size() && index < _state->submeshes.size())
    {
        _state->stamps[index] = MaterialStamp::capture(_state->submeshes[index].material);
    }
}


std::shared_ptr<draw::MaterialBase> DrawableInstanceComponent::drawMaterial(uint32_t index) const
{
    return _state->materials.at(index);
}


void DrawableInstanceComponent::update(float)
{
    try
    {
        if (needsPreparation())
        {
            if (auto* loop = app::MainLoop::current())
            {
                loop->requestSceneFrame();
            }
        }
    }
    catch (...)
    {
    }
}


void DrawableInstanceComponent::loadResources(draw::Engine& engine, SceneLoadProgress*)
try
{
    if (!_handle.valid())
    {
        if (_resourcePath.empty() || !_bindingFailure.empty())
        {
            return;
        }
        auto context = ownerNode() ? ownerNode()->drawableCacheContext() : nullptr;
        if (!context || !context->active())
        {
            context = engine.drawableCache().shared_from_this();
        }
        if (&context->engine() != &engine)
        {
            throw std::runtime_error("Instance loading context belongs to another engine");
        }
        if (!_pendingAppearance && !_state->submeshes.empty())
        {
            _appearanceBase = std::filesystem::current_path();
            _pendingAppearance = serializeReferences(_appearanceBase);
            if (!_pendingAppearance)
            {
                throw std::runtime_error("Cannot retain authored instance state for resource recovery");
            }
        }
        const auto loaded = db::loadCachedDrawableBg2(_resourcePath, *context);
        if (!loaded)
        {
            throw std::runtime_error(loaded.diagnostic.message);
        }
        for (size_t i = 0; i < _state->materialEdits.size(); ++i)
        {
            if (_state->materialEdits[i])
            {
                rememberSubmesh(static_cast<uint32_t>(i), "material", materialJson(_state->submeshes.at(i).material, _appearanceBase));
            }
        }
        const auto bound = bindResource(*context, loaded.handle, true);
        if (!bound.ok())
        {
            throw std::runtime_error(bound.message);
        }
        return;
    }
    prepareResources(engine);
}
catch (const std::exception& error)
{
    _bindingFailure = error.what();
    warn(error.what());
}
catch (...)
{
    _bindingFailure = "Cannot resolve instance geometry; reference and authored state retained";
    warn(_bindingFailure.c_str());
}


void DrawableInstanceComponent::collectDependencyIssues(const ComponentValidationContext&, std::vector<ComponentIssue>& issues) const
{
    if (!_bindingFailure.empty())
    {
        issues.push_back({"drawable-instance.binding", ComponentIssueSeverity::Warning, _bindingFailure,
            "The model reference and independent appearance were retained. Restore the source and retry loading.", {}});
    }
    else if ((!_resourcePath.empty() || _handle.valid()) && !resourcesLoaded())
    {
        issues.push_back({"drawable-instance.resources", ComponentIssueSeverity::Warning,
            "Instance geometry or material resources are unavailable.",
            "The authored instance state is retained; restore its model/textures and retry resources.", {}});
    }
}


std::shared_ptr<Component> DrawableInstanceComponent::clone() const
try
{
    auto result = std::make_shared<DrawableInstanceComponent>();
    result->_state->submeshes = _state->submeshes;
    result->_state->materialEdits = _state->materialEdits;
    for (auto& submesh : result->_state->submeshes)
    {
        submesh.material = draw::cloneMaterialAttributes(submesh.material);
    }
    result->_resourcePath = resourcePath();
    result->_exportBasis = _exportBasis;
    result->_appearanceBase = _appearanceBase;
    result->_pendingAppearance = _pendingAppearance ? json::JsonParser(_pendingAppearance->serialize()).parse() : nullptr;
    result->_transform = _transform;
    result->_tracing = _tracing;
    result->_priority = _priority;
    result->_ignoreSerialization = _ignoreSerialization;
    if (_handle.valid())
    {
        result->_handle = _handle;
        result->_cache = _cache;
        const std::weak_ptr<State> weak = result->_state;
        auto use = _cache->retain(_handle, draw::DrawableUse::SceneInstance, "drawable-instance-clone:" + _handle.identifier() + ":instance-" + std::to_string(result->_debugIdentity), [weak]() noexcept
        {
            if (const auto state = weak.lock())
            {
                state->materials.clear();
                state->deviceAvailable = false;
            }
        }, [weak]()
        {
            const auto state = weak.lock();
            return state ? state->dependencies() : draw::DrawableDependencies{};
        });
        if (!use)
        {
            warn(use.diagnostic.message.c_str());
            return nullptr;
        }
        result->_use = std::move(use.lease);
        result->prepareResources(_cache->engine());
    }
    return result;
}
catch (...)
{
    warn("Cannot clone instance; previous component retained.");
    return nullptr;
}


std::shared_ptr<DrawableInstanceComponent> DrawableInstanceComponent::cloneForResourceReplacement(const draw::Drawable& resource) const
{
    if (!_handle.valid() || !resource.isLoaded() || resource.engine() != &_cache->engine())
    {
        throw std::invalid_argument("Instance replacement preparation requires this live engine and resource identity");
    }
    if (!resourcesLoaded())
    {
        throw std::invalid_argument("Resolve unavailable instance materials before replacing shared geometry");
    }
    if (_state->submeshes.size() != resource.submeshesCount())
    {
        warn("Replacement topology differs; preserving shared indices, truncating excess state and appending resource defaults. Undo retains the prior appearance.");
    }
    auto result = std::make_shared<DrawableInstanceComponent>();
    result->_cache = _cache;
    result->_handle = _handle;
    result->_resourcePath = resourcePath();
    result->_exportBasis = _exportBasis;
    result->_transform = _transform;
    result->_tracing = _tracing;
    result->_priority = _priority;
    result->_ignoreSerialization = _ignoreSerialization;
    draw::ImageLoadContext images(resource.engine());
    for (uint32_t sub = 0; sub < resource.submeshesCount(); ++sub)
    {
        draw::Drawable::SubmeshAttributes value;
        if (sub < _state->submeshes.size())
        {
            value = _state->submeshes[sub];
        }
        else
        {
            value = {resource.materialDefinition(sub), resource.localSubmeshTransform(sub), resource.submeshName(sub),
                resource.submeshGroupName(sub), resource.submeshVisibility(sub)};
        }
        value.material = draw::cloneMaterialAttributes(value.material);
        auto material = sub < _state->materials.size() ? _state->materials[sub]->snapshot() :
            std::make_shared<draw::MaterialBase>(resource.engine());
        material->setMaterialAttributes(value.material);
        material->updateTextures(images);
        (void)draw::packMaterial(*material);
        result->_state->stamps.push_back(MaterialStamp::capture(value.material));
        result->_state->submeshes.push_back(std::move(value));
        result->_state->materials.push_back(std::move(material));
    }
    const std::weak_ptr<State> weak = result->_state;
    auto use = _cache->retain(_handle, draw::DrawableUse::SceneInstance,
        "replacement-instance:" + _handle.identifier() + ":instance-" + std::to_string(result->_debugIdentity), [weak]() noexcept
    {
        if (const auto state = weak.lock())
        {
            state->materials.clear();
            state->deviceAvailable = false;
        }
    }, [weak]()
    {
        const auto state = weak.lock();
        return state ? state->dependencies() : draw::DrawableDependencies{};
    });
    if (!use)
    {
        throw std::runtime_error(use.diagnostic.message);
    }
    result->_use = std::move(use.lease);
    result->_state->deviceAvailable = true;
    if (_handle.geometryRevision() == std::numeric_limits<uint64_t>::max())
    {
        throw std::overflow_error("Drawable geometry revision exhausted");
    }
    result->_revision = _handle.geometryRevision() + 1;
    result->_ready = true;
    result->_retry = false;
    return result;
}


bool DrawableInstanceComponent::synchronizeResourceRevision(const draw::Drawable& expected) noexcept
{
    if (renderResource().get() != &expected || !_ready || !_state->deviceAvailable ||
        _state->submeshes.size() != expected.submeshesCount() || _state->materials.size() != _state->submeshes.size() ||
        _state->stamps.size() != _state->submeshes.size())
    {
        return false;
    }
    for (size_t i = 0; i < _state->stamps.size(); ++i)
    {
        if (!(_state->stamps[i] == MaterialStamp::capture(_state->submeshes[i].material)))
        {
            return false;
        }
    }
    _revision = _handle.geometryRevision();
    _retry = false;
    return true;
}


std::filesystem::path DrawableInstanceComponent::resourcePath() const
{
    if (_handle.valid())
    {
        const auto info = _cache->inspect(_handle);
        if (info)
        {
            if (!info->exportPath.empty() &&
                (std::find(info->lookupAliases.begin(), info->lookupAliases.end(), info->exportPath) != info->lookupAliases.end() ||
                    std::any_of(info->publishedExports.begin(), info->publishedExports.end(), [&](const auto& value)
                    {
                        return value.path == info->exportPath;
                    })))
            {
                return info->exportPath;
            }
            if (info->kind == draw::DrawableResourceKind::Bg2)
            {
                return info->sourcePath;
            }
        }
    }
    return _resourcePath;
}


void DrawableInstanceComponent::rememberField(const char* key, std::shared_ptr<json::JsonNode> value)
{
    if (_pendingAppearance)
    {
        _pendingAppearance->objectValue()[key] = std::move(value);
    }
}


void DrawableInstanceComponent::rememberSubmesh(uint32_t index, const char* key, std::shared_ptr<json::JsonNode> value)
{
    if (!_pendingAppearance)
    {
        return;
    }
    auto& object = _pendingAppearance->objectValue();
    if (!json::ObjectReader(_pendingAppearance).getArray("submeshes"))
    {
        object["submeshes"] = json::JSON(json::JsonList{});
    }
    auto& list = object["submeshes"]->listValue();
    while (list.size() <= index)
    {
        list.push_back(json::JSON(json::JsonObject{}));
    }
    if (!list[index] || !list[index]->isObject())
    {
        list[index] = json::JSON(json::JsonObject{});
    }
    list[index]->objectValue()[key] = std::move(value);
}


void DrawableInstanceComponent::overlayAppearance(const std::shared_ptr<json::JsonNode>& data,
    const std::filesystem::path& base)
{
    const json::ObjectReader reader(data);
    const auto invalid = [&](const json::ObjectReader& object, const char* key, bool valid)
    {
        if (!object.isUndefined(key) && !valid)
        {
            warn((std::string("Malformed instance field '") + key + "'; original/default value retained.").c_str());
        }
    };
    const auto matrix = [&](const json::ObjectReader& object, glm::mat4& target)
    {
        const auto value = object.getGlmMat4("transform");
        if (value && validTransform(*value))
        {
            target = *value;
        }
        else
        {
            invalid(object, "transform", false);
        }
    };
    matrix(reader, _transform);
    if (const auto value = reader.getBool("rayTracingEnabled"))
    {
        _tracing = *value;
    }
    invalid(reader, "rayTracingEnabled", bool(reader.getBool("rayTracingEnabled")));
    const auto submeshes = reader.getArray("submeshes");
    invalid(reader, "submeshes", bool(submeshes));
    if (!submeshes)
    {
        return;
    }
    if (_state->submeshes.size() != submeshes->size())
    {
        warn("Instance submesh count differs from geometry; surplus ignored and missing values use originals.");
    }
    for (size_t i = 0; i < std::min(_state->submeshes.size(), submeshes->size()); ++i)
    {
        const auto item = submeshes->getObject(i);
        if (!item)
        {
            warn("Malformed instance submesh; original/default state retained.");
            continue;
        }
        auto& target = _state->submeshes[i];
        matrix(*item, target.transform);
        if (const auto value = item->getString("name"))
        {
            target.name = *value;
        }
        invalid(*item, "name", bool(item->getString("name")));
        if (const auto value = item->getString("groupName"))
        {
            target.groupName = *value;
        }
        invalid(*item, "groupName", bool(item->getString("groupName")));
        if (const auto value = item->getBool("visible"))
        {
            target.visible = *value;
        }
        invalid(*item, "visible", bool(item->getBool("visible")));
        const auto material = item->getObject("material");
        invalid(*item, "material", bool(material));
        if (!material)
        {
            continue;
        }
        const auto original = target.material;
        auto object = material->node()->objectValue();
        const auto check = [&](const char* key, bool valid)
        {
            invalid(*material, key, valid);
            if (!valid)
            {
                object.erase(key);
            }
        };
        for (const auto* key : {"name", "groupName"})
        {
            check(key, bool(material->getString(key)));
        }
        for (const auto* key : {"isTransparent", "doubleSided", "isSolid", "visible", "isUnlit", "unlit",
            "metalnessInvert", "roughnessInvert", "lightEmissionInvert"})
        {
            check(key, bool(material->getBool(key)));
        }
        for (const auto* key : {"refractionFactor", "alphaCutoff", "metalness", "roughness", "sheenIntensity", "lightEmission"})
        {
            check(key, bool(material->getNumber(key)));
        }
        check("albedo", material->isVec3("albedo") || material->isVec4("albedo"));
        for (const auto* key : {"fresnelTint", "sheenColor"})
        {
            check(key, material->isColor(key));
        }
        for (const auto* key : {"albedoScale", "metalnessScale", "roughnessScale", "normalScale", "ambientOcclussionScale", "lightEmissionScale"})
        {
            check(key, material->isVec2(key));
        }
        for (const auto* key : {"albedoUV", "metalnessUV", "roughnessUV", "normalUV", "ambientOcclussionUV", "lightEmissionUV",
            "metalnessChannel", "roughnessChannel", "ambientOcclussionChannel", "lightEmissionChannel"})
        {
            const auto value = material->getInteger<uint32_t>(key);
            const bool channel = std::string(key).find("Channel") != std::string::npos;
            check(key, value && *value <= (channel ? 3u : 1u));
        }
        const auto mode = material->getString("indirectTextureMode");
        check("indirectTextureMode", mode && (*mode == "ao" || *mode == "bakedAO" || *mode == "bakedGI"));
        for (size_t slot = 0; slot < 6; ++slot)
        {
            const auto* key = TextureKeys[slot];
            if (material->isNull(key))
            {
                (target.material.*TextureSetters[slot])(nullptr);
                object.erase(key);
            }
            else
            {
                const auto path = material->getString(key);
                check(key, path && !path->empty());
            }
            const auto settingsKey = std::string(key) + "Settings";
            const auto settings = material->getObject(settingsKey);
            invalid(*material, settingsKey.c_str(), bool(settings));
            if (!settings)
            {
                object.erase(settingsKey);
                continue;
            }
            auto repaired = settings->node()->objectValue();
            for (const auto* field : {"mag", "min", "u", "v", "w", "color"})
            {
                const auto value = settings->getInteger<uint32_t>(field);
                const std::string name(field);
                const auto maximum = name == "mag" || name == "min" ? 1u : name == "color" ? 2u : 3u;
                if (!value || *value > maximum || (name == "color" && *value == 0))
                {
                    invalid(*settings, field, false);
                    repaired.erase(field);
                }
            }
            for (const auto* field : {"minLod", "maxLod", "bias"})
            {
                if (!settings->getNumber(field))
                {
                    invalid(*settings, field, false);
                    repaired.erase(field);
                }
            }
            if (!settings->getBool("mipmaps"))
            {
                invalid(*settings, "mipmaps", false);
                repaired.erase("mipmaps");
            }
            object[settingsKey] = json::JSON(repaired);
        }
        try
        {
            utils::MaterialSerializer serializer;
            (void)serializer.deserializeMaterial(json::JSON(object)->serialize(), base, target.material);
            // The conventional parser supplies source defaults. Instance overlays
            // retain the resource controls whenever JSON omitted those fields.
            if (!object.contains("albedoScale"))
            {
                target.material.setAlbedoScale(original.albedoScale());
            }
            if (!object.contains("albedoUV"))
            {
                target.material.setAlbedoUVSet(original.albedoUVSet());
            }
            if (!object.contains("metalnessScale"))
            {
                target.material.setMetalnessScale(original.metalnessScale());
            }
            if (!object.contains("metalnessUV"))
            {
                target.material.setMetalnessUVSet(original.metalnessUVSet());
            }
            if (!object.contains("roughnessScale"))
            {
                target.material.setRoughnessScale(original.roughnessScale());
            }
            if (!object.contains("roughnessUV"))
            {
                target.material.setRoughnessUVSet(original.roughnessUVSet());
            }
            if (!object.contains("normalScale"))
            {
                target.material.setNormalScale(original.normalScale());
            }
            if (!object.contains("normalUV"))
            {
                target.material.setNormalUVSet(original.normalUVSet());
            }
            if (!object.contains("ambientOcclussionScale"))
            {
                target.material.setAoScale(original.aoScale());
            }
            if (!object.contains("ambientOcclussionUV"))
            {
                target.material.setAoUVSet(original.aoUVSet());
            }
            if (!object.contains("metalnessChannel"))
            {
                target.material.setMetalnessChannel(original.metalnessChannel());
            }
            if (!object.contains("roughnessChannel"))
            {
                target.material.setRoughnessChannel(original.roughnessChannel());
            }
            if (!object.contains("ambientOcclussionChannel"))
            {
                target.material.setAoChannel(original.aoChannel());
            }
        }
        catch (...)
        {
            warn("Cannot restore an instance material field; recovered/default fields retained.");
        }
    }
}


void DrawableInstanceComponent::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath)
try
{
    const auto released = clearResource();
    if (!released.ok())
    {
        warn(released.message.c_str());
        return;
    }
    deserializeCommon(data);
    const json::ObjectReader reader(data);
    _appearanceBase = std::filesystem::absolute(basePath.empty() ? "." : basePath).lexically_normal();
    _pendingAppearance = json::JsonParser(data->serialize()).parse();
    relocateMaterials(_pendingAppearance, _appearanceBase, _appearanceBase);
    if (const auto path = reader.getString("path"); path && !path->empty())
    {
        _resourcePath = std::filesystem::absolute(_appearanceBase / *path).lexically_normal();
    }
    else
    {
        warn("Instance model reference is missing or malformed; authored appearance retained.");
    }
    _transform = glm::mat4(1);
    _tracing = true;
    const auto submeshes = reader.getArray("submeshes");
    _state->submeshes.clear();
    _state->materialEdits.clear();
    _state->submeshes.resize(submeshes ? submeshes->size() : 0);
    overlayAppearance(_pendingAppearance, _appearanceBase);
    retryResources();
}
catch (...)
{
    warn("Invalid instance representation; recovered/default state retained.");
}


std::shared_ptr<json::JsonNode> DrawableInstanceComponent::serializeReferences(const std::filesystem::path& basePath)
try
{
    const auto path = resourcePath();
    if (path.empty() && !_pendingAppearance)
    {
        warn("Instance needs a model export destination; use a complete scene save.");
        return nullptr;
    }
    const auto base = std::filesystem::absolute(basePath.empty() ? "." : basePath).lexically_normal();
    auto result = Component::serialize(basePath);
    if (_pendingAppearance)
    {
        for (size_t i = 0; i < _state->materialEdits.size(); ++i)
        {
            if (_state->materialEdits[i])
            {
                rememberSubmesh(static_cast<uint32_t>(i), "material", materialJson(_state->submeshes.at(i).material, _appearanceBase));
            }
        }
        auto pending = json::JsonParser(_pendingAppearance->serialize()).parse();
        relocateMaterials(pending, _appearanceBase, base);
        const json::ObjectReader reader(pending);
        for (const auto* key : {"transform", "rayTracingEnabled", "submeshes"})
        {
            if (const auto value = reader.getNode(key))
            {
                result->objectValue()[key] = value;
            }
        }
    }
    else
    {
        auto basis = path == _resourcePath ? _exportBasis : std::vector<glm::mat4>{};
        if (_handle.valid())
        {
            if (const auto info = _cache->inspect(_handle))
            {
                for (const auto& published : info->publishedExports)
                {
                    if (published.path == path)
                    {
                        basis = published.submeshTransforms;
                        break;
                    }
                }
            }
        }
        auto submeshes = json::JSON(json::JsonList{});
        for (size_t sub = 0; sub < _state->submeshes.size(); ++sub)
        {
            const auto& value = _state->submeshes[sub];
            const auto transform = sub < basis.size() ? value.transform * glm::inverse(basis[sub]) : value.transform;
            submeshes->listValue().push_back(json::JSON(json::JsonObject{
                {"material", materialJson(value.material, base)}, {"transform", json::JSON(transform)},
                {"name", json::JSON(value.name)}, {"groupName", json::JSON(value.groupName)}, {"visible", json::JSON(value.visible)}}));
        }
        result->objectValue()["submeshes"] = submeshes;
        result->objectValue()["transform"] = json::JSON(_transform);
        result->objectValue()["rayTracingEnabled"] = json::JSON(_tracing);
    }
    if (!path.empty())
    {
        const auto relative = path.lexically_relative(base);
        result->objectValue()["path"] = json::JSON((relative.empty() ? path : relative).generic_string());
    }
    else if (_pendingAppearance)
    {
        if (const auto unresolved = json::ObjectReader(_pendingAppearance).getNode("path"))
        {
            result->objectValue()["path"] = unresolved;
        }
    }
    return result;
}
catch (const std::exception& error)
{
    warn(error.what());
    return nullptr;
}
catch (...)
{
    warn("Cannot serialize instance appearance; save refused.");
    return nullptr;
}


std::shared_ptr<json::JsonNode> DrawableInstanceComponent::serialize(const std::filesystem::path& basePath)
{
    return serializeReferences(basePath);
}


std::shared_ptr<json::JsonNode> DrawableInstanceComponent::serializeExportReferences(const std::filesystem::path& basePath,
    const std::filesystem::path& modelPath, const std::vector<base::MaterialAttributes>& materials,
    const std::vector<glm::mat4>& localTransforms) const
{
    if (materials.size() != _state->submeshes.size() || localTransforms.size() != materials.size() || modelPath.empty())
    {
        throw std::invalid_argument("Instance export view must cover every authored submesh and a model destination");
    }
    const auto base = std::filesystem::absolute(basePath.empty() ? "." : basePath).lexically_normal();
    // Component::serialize is non-const for legacy custom serializers. This base
    // implementation only reads type/priority; the export view changes no state.
    auto result = const_cast<DrawableInstanceComponent*>(this)->Component::serialize(basePath);
    auto submeshes = json::JSON(json::JsonList{});
    for (size_t i = 0; i < materials.size(); ++i)
    {
        const auto& value = _state->submeshes[i];
        submeshes->listValue().push_back(json::JSON(json::JsonObject{
            {"material", materialJson(materials[i], base)}, {"transform", json::JSON(localTransforms[i])},
            {"name", json::JSON(value.name)}, {"groupName", json::JSON(value.groupName)}, {"visible", json::JSON(value.visible)}}));
    }
    result->objectValue()["submeshes"] = submeshes;
    result->objectValue()["transform"] = json::JSON(_transform);
    result->objectValue()["rayTracingEnabled"] = json::JSON(_tracing);
    const auto relative = modelPath.lexically_relative(base);
    result->objectValue()["path"] = json::JSON((relative.empty() ? modelPath : relative).generic_string());
    return result;
}


void DrawableInstanceComponent::recordExportPublication(const std::filesystem::path& path,
    const std::vector<glm::mat4>& basis)
{
    if (path.empty() || (_handle.valid() && basis.size() != renderResource()->submeshesCount()) ||
        std::any_of(basis.begin(), basis.end(), [](const auto& value)
        {
            return !validTransform(value);
        }))
    {
        throw std::invalid_argument("Instance publication needs a model path and a complete finite export basis");
    }
    auto reference = std::filesystem::absolute(path).lexically_normal();
    auto transforms = basis;
    _resourcePath.swap(reference);
    _exportBasis.swap(transforms);
}


BG2E_SCENE_REGISTER_COMPONENT(DrawableInstanceComponent);

}
