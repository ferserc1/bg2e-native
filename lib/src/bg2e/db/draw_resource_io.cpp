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

#include <bg2e/db/scene.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/draw/MaterialBase.hpp>
#include <bg2e/scene/ChainJoint.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/DrawableInstanceComponent.hpp>
#include <bg2e/utils/MaterialSerializer.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/scene/TextComponent.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/utils/utils.hpp>
#include "../base/TextDiagnostics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace bg2e::db {
namespace {
void warn(SceneSaveReport& report, const std::string& message)
{
    report.warnings.push_back(message);
    std::fprintf(stderr, "Partial scene save: %s\n", message.c_str());
}

using Representations = std::map<const scene::Component*, std::shared_ptr<json::JsonNode>>;

// Keep custom component serializers authoritative, but isolate their failures
// so one broken resource cannot discard unrelated nodes and authored edits.
std::shared_ptr<json::JsonNode> serializeRecoverable(scene::Node& node,
    const std::filesystem::path& base, scene::SceneSaveProgress* progress, SceneSaveReport& report,
    const Representations* overrides = nullptr)
{
    using namespace json;
    if (progress)
    {
        if (progress->callback)
        {
            try
            {
                progress->callback(node.name(), progress->saved, progress->total);
            }
            catch (...)
            {
                warn(report, "Scene save progress callback failed");
            }
        }
        ++progress->saved;
    }
    JsonList components;
    for (const auto& [type, component] : node.components())
    {
        if (component->ignoreSerialization())
        {
            continue;
        }
        try
        {
            const auto prepared = overrides ? overrides->find(component.get()) : Representations::const_iterator{};
            auto value = overrides && prepared != overrides->end() ? prepared->second : component->serializeReferences(base);
            if (!value || !ObjectReader(value).isValid())
            {
                throw std::runtime_error("Component serializer returned no object");
            }
            components.push_back(value);
        }
        catch (const std::exception& error)
        {
            warn(report, node.name() + "/" + component->typeName() + ": " + error.what());
        }
        catch (...)
        {
            warn(report, node.name() + "/" + component->typeName() + ": unknown serialization error");
        }
    }
    JsonList children;
    for (const auto& child : node.children())
    {
        children.push_back(serializeRecoverable(*child, base, progress, report, overrides));
    }
    return JSON(JsonObject{{"type", JSON("Node")}, {"name", JSON(node.name())},
        {"enabled", JSON(node.enabled())}, {"steady", JSON(node.steady())},
        {"components", JSON(components)}, {"children", JSON(children)}});
}

std::shared_ptr<json::JsonNode> sceneDocument(scene::Node& root,
    const std::filesystem::path& base, scene::SceneProgressCallback callback, SceneSaveReport& report,
    const Representations* overrides = nullptr)
{
    scene::SceneSaveProgress progress;
    progress.callback = callback;
    const auto count = [](auto&& self, const scene::Node& node) -> int
    {
        int result = 1;
        for (const auto& child : node.children())
        {
            result += self(self, *child);
        }
        return result;
    };
    progress.total = count(count, root);
    using namespace json;
    auto representation = serializeRecoverable(root, base, callback ? &progress : nullptr, report, overrides);
    return JSON(JsonObject{{"fileType", JSON("bg2e::scene")},
        {"version", JSON(JsonObject{{"major", JSON(1)}, {"minor", JSON(0)}, {"rev", JSON(0)}})},
        {"scene", JSON(JsonList{representation})}});
}

// Stage every output, install dependencies before the main document, and retain
// rollback copies until the main replacement succeeds. Only explicitly planned
// intended targets may be overwritten; collision copies use exclusive creation.
class Publication {
  public:
    explicit Publication(const std::filesystem::path& target)
        : destination(std::filesystem::absolute(target).lexically_normal()), prefix(utils::uniqueId())
    {
        if (target.empty() || destination.filename().empty() || destination.filename() == "." || destination.filename() == "..")
        {
            throw std::invalid_argument("Export requires an output filename");
        }
        std::filesystem::create_directories(destination.parent_path());
        for (unsigned attempt = 0; attempt < 32; ++attempt)
        {
            directory = destination.parent_path() / (".bg2e-save-" + prefix);
            std::error_code error;
            if (std::filesystem::create_directory(directory, error))
            {
                return;
            }
            if (error && error != std::errc::file_exists)
            {
                throw std::filesystem::filesystem_error("Could not create export staging directory", directory, error);
            }
            prefix = utils::uniqueId();
        }
        throw std::runtime_error("Could not allocate a unique export staging directory");
    }

    ~Publication()
    {
        std::error_code error;
        if (!committed)
        {
            for (auto it = installed.rbegin(); it != installed.rend(); ++it)
            {
                if (it->backup.empty())
                {
                    std::filesystem::remove(it->target, error);
                }
                else
                {
                    try
                    {
                        replace(it->backup, it->target);
                    }
                    catch (...)
                    {
                        // Keep the backup available for recovery if the OS also
                        // refuses rollback (permissions/media changed mid-save).
                        keepStaging = true;
                        std::fprintf(stderr, "Could not restore export file; backup retained at %s\n", it->backup.string().c_str());
                    }
                }
            }
        }
        if (!keepStaging)
        {
            std::filesystem::remove_all(directory, error);
        }
    }

    std::filesystem::path resource(const std::string& extension)
    {
        std::filesystem::path result;
        do
        {
            result = directory / (prefix + "-" + std::to_string(sequence++) + extension);
        }
        while (std::filesystem::exists(result) || std::filesystem::exists(destination.parent_path() / result.filename()));
        reserved.insert(result.filename());
        return result;
    }

    std::filesystem::path model(const std::filesystem::path& intended, bool allowOverwrite)
    {
        auto name = intended.filename();
        if (name.empty() || (name.extension() != ".bg2" && name.extension() != ".vwglb"))
        {
            return resource(".bg2");
        }
        auto target = destination.parent_path() / name;
        const bool own = allowOverwrite && std::filesystem::absolute(intended).lexically_normal() == target;
        if (target == destination || reserved.contains(name) || (!own && std::filesystem::exists(target)))
        {
            return resource(".bg2");
        }
        reserved.insert(name);
        if (own)
        {
            replacements.insert(name);
        }
        return directory / name;
    }

    std::filesystem::path imageResource(const std::filesystem::path& source)
    {
        return externalResource(source);
    }

    std::filesystem::path externalResource(const std::filesystem::path& source)
    {
        const auto key = std::filesystem::weakly_canonical(std::filesystem::absolute(source));
        if (const auto found = external.find(key); found != external.end())
        {
            return found->second;
        }
        auto target = resource(source.extension().string());
        if (key.parent_path() == destination.parent_path() && key.filename() != destination.filename() &&
            !reserved.contains(key.filename()))
        {
            target = directory / key.filename();
        }
        try
        {
            std::filesystem::copy_file(source, target);
        }
        catch (...)
        {
            discard(target);
            throw;
        }
        reserved.insert(target.filename());
        if (key == destination.parent_path() / target.filename())
        {
            replacements.insert(target.filename());
        }
        external.emplace(key, target);
        return target;
    }

    void discard(const std::filesystem::path& staged) noexcept
    {
        std::error_code error;
        std::filesystem::remove(staged, error);
    }

    void commit(const std::filesystem::path& main)
    {
        std::vector<std::filesystem::path> resources;
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path() != main)
            {
                resources.push_back(entry.path());
            }
        }
        std::stable_sort(resources.begin(), resources.end(), [](const auto& a, const auto& b)
        {
            const auto modelFile = [](const auto& path)
            {
                return path.extension() == ".bg2" || path.extension() == ".vwglb";
            };
            return modelFile(a) < modelFile(b);
        });
        installed.reserve(resources.size());
        const auto backups = directory / ".rollback";
        std::filesystem::create_directory(backups);
        for (const auto& source : resources)
        {
            const auto target = destination.parent_path() / source.filename();
            std::filesystem::path backup;
            if (replacements.contains(source.filename()) && std::filesystem::exists(target))
            {
                if (!std::filesystem::is_regular_file(target) || std::filesystem::is_symlink(target))
                {
                    throw std::runtime_error("Export refuses to replace a non-regular resource: " + target.string());
                }
                backup = backups / source.filename();
                std::filesystem::copy_file(target, backup);
            }
            installed.push_back({target, backup});
            if (replacements.contains(source.filename()))
            {
                replace(source, target);
            }
            else
            {
                std::error_code error;
                std::filesystem::create_hard_link(source, target, error);
                if (error)
                {
#ifdef _WIN32
                    auto* output = _wfopen(target.c_str(), L"wbx");
#else
                    auto* output = std::fopen(target.c_str(), "wbx");
#endif
                    if (!output)
                    {
                        installed.pop_back(); // The existing target is not ours.
                        throw std::runtime_error("Could not reserve export resource: " + target.string());
                    }
                    if (std::fclose(output) != 0)
                    {
                        throw std::runtime_error("Could not close export resource: " + target.string());
                    }
                    std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing);
                }
            }
        }
        replace(main, destination);
        committed = true;
    }

    std::filesystem::path destination;
    std::filesystem::path directory;

  private:
    struct Installed {
        std::filesystem::path target;
        std::filesystem::path backup;
    };

    static void replace(const std::filesystem::path& source, const std::filesystem::path& target)
    {
#ifdef _WIN32
        if (!MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Publish export file");
        }
#else
        std::filesystem::rename(source, target);
#endif
    }

    std::string prefix;
    size_t sequence = 0;
    bool committed = false;
    bool keepStaging = false;
    std::vector<Installed> installed;
    std::set<std::filesystem::path> replacements;
    std::set<std::filesystem::path> reserved;
    std::map<std::filesystem::path, std::filesystem::path> external;
};

struct ReferenceRestore {
    std::vector<std::pair<base::MaterialAttributes*, base::MaterialAttributes>> materials;

    ~ReferenceRestore()
    {
        for (auto& [material, attributes] : materials)
        {
            using std::swap;
            swap(*material, attributes);
        }
    }
};

using TextureGetter = std::shared_ptr<base::Texture> (base::MaterialAttributes::*)() const;
const TextureGetter Getters[] = {
    &base::MaterialAttributes::albedoTexture, &base::MaterialAttributes::metalnessTexture,
    &base::MaterialAttributes::roughnessTexture, &base::MaterialAttributes::normalTexture,
    &base::MaterialAttributes::aoTexture, &base::MaterialAttributes::lightEmissionTexture};

using TextureSetter = void (base::MaterialAttributes::*)(std::shared_ptr<base::Texture>);
const TextureSetter Setters[] = {
    &base::MaterialAttributes::setAlbedoTexture, &base::MaterialAttributes::setMetalnessTexture,
    &base::MaterialAttributes::setRoughnessTexture, &base::MaterialAttributes::setNormalTexture,
    &base::MaterialAttributes::setAoTexture, &base::MaterialAttributes::setLightEmissionTexture};

enum class MaterialPaths { Staging, Published };

base::MaterialAttributes packageMaterial(const base::MaterialAttributes& authored, draw::Engine& engine,
    Publication& publication, const std::shared_ptr<draw::MaterialBase>& live = nullptr,
    MaterialPaths paths = MaterialPaths::Published)
{
    auto result = draw::cloneMaterialAttributes(authored);
    for (uint32_t slot = 0; slot < 6; ++slot)
    {
        const auto source = (authored.*Getters[slot])();
        if (!source)
        {
            continue;
        }
        if (source->ignoreSerialization())
        {
            (result.*Setters[slot])(nullptr);
            continue;
        }
        auto settings = draw::cloneTextureSettings(*source);
        std::filesystem::path staged;
        if (source->type() == base::Texture::TypeFilesystem && !source->imageFilePath().empty())
        {
            staged = publication.imageResource(source->imageFilePath());
        }
        else if (settings->proceduralTextureGenerator())
        {
            std::unique_ptr<base::Image> pixels(settings->generateImage());
            if (!pixels || !pixels->isValid())
            {
                throw std::runtime_error("Procedural material image is unavailable");
            }
            staged = publication.resource(".png");
            try
            {
                saveImage(staged, pixels->data(), pixels->width(), pixels->height(), pixels->channels());
            }
            catch (...)
            {
                publication.discard(staged);
                throw;
            }
        }
        else
        {
            if (!live || !authored.isUpdated() || !live->texturesUpdated())
            {
                throw std::runtime_error("Material image has no recoverable current pixels");
            }
            const auto texture = live->texture(static_cast<draw::TextureSlot>(slot));
            if (!texture || texture->engine() != &engine || !texture->image())
            {
                throw std::runtime_error("Material image belongs to another context or is unavailable");
            }
            staged = publication.resource(texture->image()->pixelFormat() == gpu::PixelFormat::R32G32B32A32_SFLOAT ? ".hdr" : ".png");
            try
            {
                saveImage(staged, *texture->image());
            }
            catch (...)
            {
                publication.discard(staged);
                throw;
            }
        }
        // The BG2 writer reads/copies these images while staging the model.
        // JSON references instead address their location after publication.
        const auto reference = paths == MaterialPaths::Staging ? staged : publication.destination.parent_path() / staged.filename();
        settings->setImageFilePath(reference.string());
        (result.*Setters[slot])(settings);
    }
    return result;
}


glm::mat4 checkedTransform(const glm::mat4& value)
{
    for (int c = 0; c < 4; ++c)
    {
        for (int r = 0; r < 4; ++r)
        {
            if (!std::isfinite(value[c][r]))
            {
                throw std::invalid_argument("Cannot export a non-finite drawable transform");
            }
        }
    }
    const auto determinant = glm::determinant(glm::mat3(value));
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f ||
        value[0][3] != 0 || value[1][3] != 0 || value[2][3] != 0 || value[3][3] != 1)
    {
        throw std::invalid_argument("Cannot export a singular or non-affine drawable transform");
    }
    return value;
}


Bg2Mesh snapshotResource(const draw::Drawable& resource, const std::vector<draw::Drawable::SubmeshAttributes>& properties,
    const glm::mat4& globalTransform, draw::Engine& engine, Publication& publication, bool ownerProperties)
{
    if (!resource.isLoaded() || resource.engine() != &engine || !resource.mesh() ||
        properties.size() != resource.submeshesCount())
    {
        throw std::invalid_argument("Complete export requires matching geometry and properties from this draw Engine");
    }
    Bg2Mesh model;
    model.mesh = std::make_shared<geo::MeshPNUUT>();
    for (uint32_t sub = 0; sub < properties.size(); ++sub)
    {
        const auto& property = properties[sub];
        auto material = packageMaterial(property.material, engine, publication,
            ownerProperties ? resource.drawMaterial(sub) : nullptr, MaterialPaths::Staging);
        material.setGroupName(property.groupName);
        material.setVisible(property.visible);
        model.materials.push_back(std::move(material));
        model.submeshNames.push_back(property.name);
        const auto transform = checkedTransform(globalTransform * property.transform);
        const auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
        const auto range = resource.submeshData(sub);
        const auto first = static_cast<uint32_t>(model.mesh->indices.size());
        for (uint32_t i = 0; i < range.indexCount; ++i)
        {
            auto vertex = resource.mesh()->vertices.at(resource.mesh()->indices.at(range.firstIndex + i));
            vertex.position = glm::vec3(transform * glm::vec4(vertex.position, 1));
            vertex.normal = normalMatrix * vertex.normal;
            if (glm::length(vertex.normal) > 1e-8f)
            {
                vertex.normal = glm::normalize(vertex.normal);
            }
            vertex.tangent = glm::vec3(0);
            model.mesh->indices.push_back(static_cast<uint32_t>(model.mesh->vertices.size()));
            model.mesh->vertices.push_back(vertex);
        }
        model.mesh->submeshes.push_back({first, range.indexCount});
    }
    return model;
}


std::vector<draw::Drawable::SubmeshAttributes> ownerProperties(const draw::Drawable& drawable)
{
    std::vector<draw::Drawable::SubmeshAttributes> result;
    for (uint32_t sub = 0; sub < drawable.submeshesCount(); ++sub)
    {
        result.push_back({drawable.material(sub), drawable.submeshTransform(sub), drawable.submeshName(sub),
            drawable.submeshGroupName(sub), drawable.submeshVisibility(sub)});
    }
    return result;
}


void copyJoints(scene::Node& node, Bg2Mesh& model)
{
    if (const auto joint = node.getComponent<scene::InputChainJointComponent>())
    {
        model.inputJoint = joint->joint();
    }
    if (const auto joint = node.getComponent<scene::OutputChainJointComponent>())
    {
        model.outputJoint = joint->joint();
    }
}


Bg2Mesh snapshot(scene::Node& node, draw::Engine& engine, Publication& publication)
{
    const auto owner = node.drawable();
    const auto drawable = owner ? owner->drawable() : nullptr;
    if (!drawable)
    {
        throw std::invalid_argument("Model export requires an authored drawable owner");
    }
    for (uint32_t sub = 0; sub < drawable->submeshesCount(); ++sub)
    {
        if (!drawable->material(sub).isUpdated())
        {
            throw std::logic_error("Resolve pending or failed texture edits before exporting a BG2 model");
        }
    }
    auto model = snapshotResource(*drawable, ownerProperties(*drawable), glm::mat4(1), engine, publication, true);
    copyJoints(node, model);
    return model;
}


// Only referenced identities enter this table. The export plan is independent
// of tree ordering, while component representations remain inline JSON today.
class ResourceExports {
  public:
    ResourceExports(scene::Node& root, draw::Engine& engine, Publication& publication, SceneSaveReport& report)
        : engine(engine), publication(publication), report(report)
    {
        collect(root);
    }

    void stage()
    {
        for (auto& entry : entries)
        {
            try
            {
                stage(entry);
            }
            catch (...)
            {
                warn(report, "Cannot stage a shared model; independent resource staging continues");
                blocked = true;
            }
        }
        for (auto& entry : entries)
        {
            try
            {
                represent(entry);
            }
            catch (...)
            {
                warn(report, "Cannot represent a shared model; independent resource serialization continues");
                blocked = true;
            }
        }
        for (auto* instance : unresolved)
        {
            try
            {
                auto value = instance->serializeReferences(publication.destination.parent_path());
                if (!value || !existingModel(instance->resourcePath()))
                {
                    throw std::runtime_error("Unresolved instance has no complete existing model file");
                }
                packageJsonMaterials(value);
                representations[instance] = std::move(value);
                warn(report, "Instance geometry could not be realized; existing model and authored representation retained");
            }
            catch (const std::exception& error)
            {
                warn(report, std::string("Cannot preserve unresolved instance: ") + error.what());
                blocked = true;
            }
        }
    }

    void publishMetadata() noexcept
    {
        for (auto& entry : entries)
        {
            if (!entry.exported)
            {
                continue;
            }
            try
            {
                if (entry.owner)
                {
                    entry.owner->setResourcePath(entry.output);
                    if (!entry.owner->assignmentDiagnostic().ok())
                    {
                        throw std::runtime_error(entry.owner->assignmentDiagnostic().message);
                    }
                }
                for (auto* instance : entry.instances)
                {
                    instance->recordExportPublication(entry.output, entry.basis);
                }
                if (entry.context && entry.handle.valid())
                {
                    const auto recorded = entry.context->recordExportPublication(entry.handle, entry.output, entry.basis);
                    if (!recorded)
                    {
                        throw std::runtime_error(recorded.diagnostic.message);
                    }
                }
            }
            catch (const std::exception& error)
            {
                try
                {
                    warn(report, std::string("Files saved; runtime export bookkeeping remains pending: ") + error.what());
                }
                catch (...)
                {
                }
            }
            catch (...)
            {
                std::fprintf(stderr, "Files saved; could not update drawable export bookkeeping.\n");
            }
        }
    }

    Representations representations;
    bool blocked = false;

  private:
    struct Entry {
        draw::DrawableResourceHandle handle;
        std::shared_ptr<draw::DrawableCache> context;
        std::optional<draw::DrawableCacheEntryInfo> info;
        std::shared_ptr<const draw::Drawable> geometry;
        draw::DrawableUsageLease use;
        scene::DrawableComponent* owner = nullptr;
        scene::Node* ownerNode = nullptr;
        std::vector<scene::DrawableInstanceComponent*> instances;
        std::filesystem::path output;
        std::vector<glm::mat4> basis;
        bool exported = false;
    };

    Entry& entryFor(const draw::DrawableResourceHandle& handle, std::shared_ptr<const draw::Drawable> geometry = nullptr)
    {
        for (auto& entry : entries)
        {
            if ((handle.valid() && handle == entry.handle) || (!handle.valid() && geometry && geometry == entry.geometry))
            {
                return entry;
            }
        }
        Entry entry;
        entry.handle = handle;
        entry.geometry = std::move(geometry);
        if (handle.valid())
        {
            entry.context = handle.context();
            if (!entry.context || &entry.context->engine() != &engine)
            {
                throw std::invalid_argument("Referenced drawable belongs to another or expired engine");
            }
            entry.info = entry.context->inspect(handle);
            entry.geometry = entry.context->resolve(handle);
            auto retained = entry.context->retain(handle, draw::DrawableUse::Job, "scene-export:" + handle.identifier());
            if (!retained)
            {
                throw std::runtime_error(retained.diagnostic.message);
            }
            entry.use = std::move(retained.lease);
        }
        entries.push_back(std::move(entry));
        return entries.back();
    }

    void collect(scene::Node& node)
    {
        for (const auto& component : node.orderedComponents())
        {
            if (component->ignoreSerialization())
            {
                continue;
            }
            try
            {
                if (auto* owner = dynamic_cast<scene::DrawableComponent*>(component.get());
                    owner && owner->isSceneGeometry() && owner->requiresBg2Export())
                {
                    auto& entry = entryFor(owner->resourceHandle(), owner->drawable());
                    entry.owner = owner;
                    entry.ownerNode = &node;
                }
                else if (auto* instance = dynamic_cast<scene::DrawableInstanceComponent*>(component.get()))
                {
                    if (instance->resourceHandle().valid())
                    {
                        entryFor(instance->resourceHandle()).instances.push_back(instance);
                    }
                    else
                    {
                        unresolved.push_back(instance);
                    }
                }
            }
            catch (const std::exception& error)
            {
                warn(report, node.name() + ": cannot retain export resource: " + error.what());
                blocked = true;
            }
        }
        for (const auto& child : node.children())
        {
            collect(*child);
        }
    }

    bool existingModel(const std::filesystem::path& path) const
    {
        try
        {
            if (path.empty() || !std::filesystem::is_regular_file(path))
            {
                return false;
            }
            std::unique_ptr<Bg2Mesh> model(loadMeshBg2(path));
            if (!model || !model->mesh || model->mesh->submeshes.empty() || model->materials.size() != model->mesh->submeshes.size())
            {
                return false;
            }
            for (const auto& texture : meshTexturePaths(path))
            {
                if (!std::filesystem::is_regular_file(path.parent_path() / texture))
                {
                    return false;
                }
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    void stage(Entry& entry)
    {
        std::filesystem::path staged;
        try
        {
            if (!entry.geometry)
            {
                throw std::runtime_error("Referenced geometry is unavailable");
            }
            auto properties = entry.owner ? ownerProperties(*entry.geometry) :
                entry.info && entry.info->originalSubmeshes ? *entry.info->originalSubmeshes :
                std::vector<draw::Drawable::SubmeshAttributes>{};
            const auto global = entry.owner ? glm::mat4(1) : entry.info ? entry.info->originalTransform : glm::mat4(1);
            auto intended = entry.owner ? entry.owner->resourcePath() : entry.info ? entry.info->exportPath : std::filesystem::path{};
            if (intended.empty() && entry.info && entry.info->kind == draw::DrawableResourceKind::Bg2)
            {
                intended = entry.info->sourcePath;
            }
            bool overwrite = entry.owner ? !entry.owner->hasPendingExport() : false;
            if (!entry.owner && entry.info)
            {
                overwrite = std::find(entry.info->lookupAliases.begin(), entry.info->lookupAliases.end(), intended) != entry.info->lookupAliases.end();
                for (const auto& value : entry.info->publishedExports)
                {
                    overwrite = overwrite || value.path == intended;
                }
            }
            staged = publication.model(intended, overwrite);
            auto model = snapshotResource(*entry.geometry, properties, global, engine, publication, entry.owner != nullptr);
            if (entry.ownerNode)
            {
                copyJoints(*entry.ownerNode, model);
            }
            for (const auto& property : properties)
            {
                entry.basis.push_back(checkedTransform(global * property.transform));
            }
            storeMeshBg2(staged, &model);
            entry.output = publication.destination.parent_path() / staged.filename();
            entry.exported = true;
        }
        catch (const std::exception& error)
        {
            publication.discard(staged);
            warn(report, std::string("Shared model export incomplete; retaining prior complete file where available: ") + error.what());
            std::vector<std::filesystem::path> candidates;
            if (entry.info)
            {
                candidates.push_back(entry.info->exportPath);
                candidates.push_back(entry.info->sourcePath);
            }
            if (entry.owner)
            {
                candidates.push_back(entry.owner->resourcePath());
            }
            entry.basis.clear();
            for (const auto& candidate : candidates)
            {
                if (!existingModel(candidate))
                {
                    continue;
                }
                entry.output = candidate;
                if (entry.geometry)
                {
                    entry.basis.resize(entry.geometry->submeshesCount(), glm::mat4(1));
                }
                if (entry.info)
                {
                    for (const auto& previous : entry.info->publishedExports)
                    {
                        if (previous.path == candidate)
                        {
                            entry.basis = previous.submeshTransforms;
                        }
                    }
                }
                return;
            }
            blocked = true;
        }
    }

    void packageJsonMaterials(const std::shared_ptr<json::JsonNode>& representation)
    {
        const auto submeshes = json::ObjectReader(representation).getArray("submeshes");
        if (!submeshes)
        {
            return;
        }
        for (size_t sub = 0; sub < submeshes->size(); ++sub)
        {
            const auto item = submeshes->getObject(sub);
            const auto material = item ? item->getObject("material") : std::nullopt;
            if (!material)
            {
                continue;
            }
            for (const auto* key : {"albedoTexture", "metalnessTexture", "roughnessTexture", "normalTexture", "ambientOcclussion", "lightEmissionTexture"})
            {
                if (const auto path = material->getString(key); path && !path->empty())
                {
                    const auto source = publication.destination.parent_path() / *path;
                    try
                    {
                        const auto staged = publication.externalResource(source);
                        material->node()->objectValue()[key] = json::JSON(staged.filename().generic_string());
                    }
                    catch (const std::exception& error)
                    {
                        if (!std::filesystem::is_regular_file(source))
                        {
                            throw;
                        }
                        warn(report, std::string("Instance image retained at its authored path: ") + error.what());
                    }
                }
            }
        }
    }

    void represent(Entry& entry)
    {
        if (entry.output.empty())
        {
            blocked = true;
            return;
        }
        const auto base = publication.destination.parent_path();
        if (entry.owner)
        {
            auto value = entry.owner->serializeExportReferences(base, entry.output);
            if (!value || !json::ObjectReader(value).isValid())
            {
                throw std::runtime_error("Owner export serializer returned no object");
            }
            representations[entry.owner] = std::move(value);
        }
        for (auto* instance : entry.instances)
        {
            try
            {
                std::vector<base::MaterialAttributes> materials;
                std::vector<glm::mat4> transforms;
                for (uint32_t sub = 0; sub < instance->submeshesCount(); ++sub)
                {
                    std::shared_ptr<draw::MaterialBase> live;
                    if (sub < instance->retainedMaterialCount())
                    {
                        live = instance->drawMaterial(sub);
                    }
                    materials.push_back(packageMaterial(instance->effectiveMaterial(sub), engine, publication, live));
                    const auto basis = sub < entry.basis.size() ? entry.basis[sub] : glm::mat4(1);
                    transforms.push_back(checkedTransform(instance->localSubmeshTransform(sub) * glm::inverse(checkedTransform(basis))));
                }
                representations[instance] = instance->serializeExportReferences(base, entry.output, materials, transforms);
            }
            catch (const std::exception& error)
            {
                warn(report, std::string("Instance export incomplete; retaining authored state: ") + error.what());
                // A failed instance image must not silently turn into null. Only
                // retain its reference representation if every referenced file
                // already exists; otherwise preserve the prior document.
                try
                {
                    auto value = instance->serializeReferences(base);
                    if (!value || (!entry.exported && !existingModel(entry.output)))
                    {
                        throw std::runtime_error("No safe instance export fallback");
                    }
                    const auto relative = entry.output.lexically_relative(base);
                    value->objectValue()["path"] = json::JSON((relative.empty() ? entry.output : relative).generic_string());
                    if (auto submeshes = json::ObjectReader(value).getArray("submeshes"))
                    {
                        for (size_t sub = 0; sub < submeshes->size(); ++sub)
                        {
                            if (auto item = submeshes->getObject(sub))
                            {
                                const auto basis = sub < entry.basis.size() ? entry.basis[sub] : glm::mat4(1);
                                item->node()->objectValue()["transform"] = json::JSON(checkedTransform(
                                    instance->localSubmeshTransform(static_cast<uint32_t>(sub)) * glm::inverse(checkedTransform(basis))));
                            }
                        }
                    }
                    packageJsonMaterials(value);
                    representations[instance] = std::move(value);
                }
                catch (...)
                {
                    blocked = true;
                }
            }
        }
    }

    draw::Engine& engine;
    Publication& publication;
    SceneSaveReport& report;
    std::vector<Entry> entries;
    std::vector<scene::DrawableInstanceComponent*> unresolved;
};

void stageGraph(scene::Node& node, draw::Engine& engine, Publication& publication, ReferenceRestore& restore,
    SceneSaveReport& report)
{
    if (auto component = node.drawableRender(); component && !component->ignoreSerialization())
    {
        try
        {
            if (auto* text = dynamic_cast<scene::TextComponent*>(component))
            {
                auto attributes = text->material();
                // Albedo is never inspected, cloned, evaluated or exported here.
                for (uint32_t slot = 1; slot < 6; ++slot)
                {
                    const auto source = (attributes.*Getters[slot])();
                    if (!source || source->ignoreSerialization())
                    {
                        continue;
                    }
                    try
                    {
                        auto settings = std::make_shared<base::Texture>(*source);
                        std::filesystem::path path;
                        if (source->type() == base::Texture::TypeFilesystem && !source->imageFilePath().empty())
                        {
                            path = publication.externalResource(source->imageFilePath());
                        }
                        else
                        {
                            std::unique_ptr<base::Image> pixels(settings->generateImage());
                            if (!pixels || !pixels->isValid())
                            {
                                warn(report, node.name() + ": omitted unavailable embedded text material image");
                                (attributes.*Setters[slot])(nullptr);
                                continue;
                            }
                            path = publication.resource(".png");
                            try
                            {
                                saveImage(path, pixels->data(), pixels->width(), pixels->height(), pixels->channels());
                            }
                            catch (...)
                            {
                                publication.discard(path);
                                throw;
                            }
                        }
                        settings->setImageFilePath((publication.destination.parent_path() / path.filename()).string());
                        (attributes.*Setters[slot])(settings);
                    }
                    catch (const std::exception& error)
                    {
                        warn(report, node.name() + "/text texture " + std::to_string(slot) + ": " + error.what());
                        (attributes.*Setters[slot])(nullptr);
                    }
                }
                // Install only at this stopped IO boundary. RAII restores the exact
                // authored material (including revisions) on success and failure.
                restore.materials.emplace_back(&text->material(), std::move(attributes));
                using std::swap;
                swap(text->material(), restore.materials.back().second);
            }
        }
        catch (const std::exception& error)
        {
            warn(report, node.name() + ": resource snapshot failed; retaining authored references: " + error.what());
        }
        catch (...)
        {
            warn(report, node.name() + ": resource snapshot failed; retaining authored references");
        }
    }
    for (const auto& child : node.children())
    {
        stageGraph(*child, engine, publication, restore, report);
    }
}

// External font/notice/environment files keep their original bytes/extensions.
// Custom components still require their own explicit export contract.
void stageExternalReferences(const std::shared_ptr<json::JsonNode>& value, Publication& publication,
    SceneSaveReport& report, bool& blocked)
{
    if (!value)
    {
        return;
    }
    if (value->isObject())
    {
        auto& object = value->objectValue();
        const auto type = object.find("type");
        const auto copyReference = [&](std::shared_ptr<json::JsonNode>& reference)
        {
            if (!reference || !reference->isString() || reference->stringValue().empty())
            {
                warn(report, "Missing external resource path");
                return;
            }
            const auto source = publication.destination.parent_path() / reference->stringValue();
            try
            {
                const auto path = publication.externalResource(source);
                reference = json::JSON(path.filename().generic_string());
            }
            catch (const std::exception& error)
            {
                // Serialization already made this relative to the final scene.
                // Retaining it allows the user to restore the missing resource.
                warn(report, source.string() + ": " + error.what());
                std::error_code statusError;
                blocked = blocked || !std::filesystem::is_regular_file(source, statusError);
            }
        };
        if (type != object.end() && type->second && type->second->isString())
        {
            if (type->second->stringValue() == "Environment")
            {
                const auto found = object.find("equirectangularTexture");
                if (found != object.end() && found->second && found->second->isString() &&
                    !found->second->stringValue().empty())
                {
                    copyReference(found->second);
                }
            }
            else if (type->second->stringValue() == "TextFonts")
            {
                const auto fonts = object.find("fonts");
                if (fonts != object.end() && fonts->second && fonts->second->isList())
                {
                    for (const auto& font : fonts->second->listValue())
                    {
                        json::ObjectReader reader(font);
                        const auto path = reader.getString("path");
                        if (!path || path->empty())
                        {
                            warn(report, "Font has no authored path");
                            continue;
                        }
                        const auto source = publication.destination.parent_path() / *path;
                        std::set<std::filesystem::path> notices;
                        if (auto supplied = reader.getArray("notices"))
                        {
                            for (size_t i = 0; i < supplied->size(); ++i)
                            {
                                auto notice = supplied->getString(i);
                                if (notice && !notice->empty())
                                {
                                    notices.insert(publication.destination.parent_path() / *notice);
                                }
                                else
                                {
                                    warn(report, "Invalid supplied font notice path");
                                }
                            }
                        }
                        if (notices.empty())
                        {
                            for (const char* name : {"OFL.txt", "LICENSE", "LICENSE.txt", "LICENSE.md",
                                "COPYING", "NOTICE", "NOTICE.txt", "README.md"})
                            {
                                const auto notice = source.parent_path() / name;
                                std::error_code error;
                                if (std::filesystem::is_regular_file(notice, error))
                                {
                                    notices.insert(notice);
                                }
                            }
                        }
                        if (notices.empty())
                        {
                            base::detail::textWarning("No supplied/adjacent font notices found for ", source.string(),
                                "; verify redistribution rights and provide required notices.");
                        }
                        json::JsonList packaged;
                        for (const auto& notice : notices)
                        {
                            auto reference = json::JSON(notice.generic_string());
                            copyReference(reference);
                            packaged.push_back(reference);
                        }
                        font->objectValue()["notices"] = json::JSON(packaged);
                        copyReference(font->objectValue().at("path"));
                    }
                }
            }
        }
        for (auto& [key, child] : object)
        {
            stageExternalReferences(child, publication, report, blocked);
        }
    }
    else if (value->isList())
    {
        for (auto& child : value->listValue())
        {
            stageExternalReferences(child, publication, report, blocked);
        }
    }
}

void preservePreviousScene(Publication& publication, SceneSaveReport& report)
{
    if (report.warnings.empty())
    {
        return;
    }
    try
    {
        if (std::filesystem::is_regular_file(publication.destination))
        {
            const auto backup = publication.resource(".previous.json");
            std::filesystem::copy_file(publication.destination, backup);
            warn(report, "Previous scene preserved at: " +
                (publication.destination.parent_path() / backup.filename()).string());
        }
    }
    catch (const std::exception& error)
    {
        warn(report, std::string("Could not preserve the previous scene copy: ") + error.what());
    }
}

void recordWarnings(const std::shared_ptr<json::JsonNode>& document, const SceneSaveReport& report)
{
    if (!report.warnings.empty())
    {
        json::JsonList warnings;
        for (const auto& warning : report.warnings)
        {
            warnings.push_back(json::JSON(warning));
        }
        document->objectValue()["saveWarnings"] = json::JSON(warnings);
    }
}
}

bool saveSceneReferences(scene::Node* root, const std::filesystem::path& filePath,
    scene::SceneProgressCallback onProgress) noexcept
{
    SceneSaveReport report;
    return saveSceneReferences(root, filePath, onProgress, report);
}

bool saveSceneReferences(scene::Node* root, const std::filesystem::path& filePath,
    scene::SceneProgressCallback onProgress, SceneSaveReport& report) noexcept
try
{
    report = {};
    if (!root || filePath.empty())
    {
        throw std::invalid_argument("Scene reference save requires a root and path");
    }
    Publication publication(filePath);
    auto document = sceneDocument(*root, publication.destination.parent_path(), onProgress, report);
    preservePreviousScene(publication, report);
    recordWarnings(document, report);
    const auto main = publication.directory / publication.destination.filename();
    writeSceneDocument(document, main);
    publication.commit(main);
    return true;
}
catch (const std::exception& error)
{
    report.error = error.what();
    std::fprintf(stderr, "Scene reference save failed: %s\n", error.what());
    return false;
}
catch (...)
{
    report.error = "Unknown scene reference save error";
    std::fprintf(stderr, "%s\n", report.error.c_str());
    return false;
}

bool saveScene(scene::Node* root, draw::Engine& engine, const std::filesystem::path& path,
    scene::SceneProgressCallback progress) noexcept
{
    SceneSaveReport report;
    return saveScene(root, engine, path, progress, report);
}

bool saveScene(scene::Node* root, draw::Engine& engine, const std::filesystem::path& path,
    scene::SceneProgressCallback progress, SceneSaveReport& report) noexcept
try
{
    report = {};
    if (!root || path.empty())
    {
        throw std::invalid_argument("Scene save requires a root and output path");
    }
    Publication publication(path);
    ReferenceRestore restore;
    if (!engine.device())
    {
        throw std::invalid_argument("Resource export requires an initialized draw Engine");
    }
    engine.device()->waitIdle();
    scene::DrawableComponent::prepareResources(root, engine);
    scene::DrawableInstanceComponent::prepareResources(root, engine);
    ResourceExports exports(*root, engine, publication, report);
    exports.stage();
    stageGraph(*root, engine, publication, restore, report);
    if (exports.blocked)
    {
        throw std::runtime_error("Export has unavailable model/image references; prior document and files preserved");
    }
    auto document = sceneDocument(*root, publication.destination.parent_path(), progress, report, &exports.representations);
    try
    {
        stageExternalReferences(document, publication, report, exports.blocked);
    }
    catch (const std::exception& error)
    {
        warn(report, std::string("External resource packaging incomplete: ") + error.what());
        exports.blocked = true;
    }
    catch (...)
    {
        warn(report, "External resource packaging incomplete: unknown error");
        exports.blocked = true;
    }
    if (exports.blocked)
    {
        throw std::runtime_error("External dependencies are unavailable; prior document and files preserved");
    }
    preservePreviousScene(publication, report);
    recordWarnings(document, report);
    const auto main = publication.directory / publication.destination.filename();
    writeSceneDocument(document, main);
    publication.commit(main);
    exports.publishMetadata();
    return true;
}
catch (const std::exception& error)
{
    report.error = error.what();
    std::fprintf(stderr, "Scene save failed: %s\n", error.what());
    return false;
}
catch (...)
{
    report.error = "Unknown scene save error";
    std::fprintf(stderr, "%s\n", report.error.c_str());
    return false;
}


bool storeDrawableBg2(const std::filesystem::path& path, draw::Engine& engine, scene::Node* node) noexcept
try
{
    if (!node || !node->children().empty() || !node->enabled() || node->steady())
    {
        throw std::invalid_argument("BG2 export requires a single model node; use a scene document for a hierarchy");
    }
    if (auto* component = node->drawable();
        (component && !component->requiresBg2Export()) || node->getComponent<scene::TextComponent>())
    {
        base::detail::textWarning("Text cannot be baked into a BG2 model; export refused.");
        return false;
    }
    for (const auto& [type, component] : node->components())
    {
        if (component->ignoreSerialization())
        {
            continue;
        }
        if (!dynamic_cast<scene::DrawableComponent*>(component.get()) &&
            !dynamic_cast<scene::TransformComponent*>(component.get()) &&
            !dynamic_cast<scene::ChainJointComponent*>(component.get()))
        {
            throw std::invalid_argument("BG2 cannot preserve this component; save a scene document");
        }
    }
    if (auto transform = node->getComponent<scene::TransformComponent>();
        transform && transform->matrix() != glm::mat4(1))
    {
        throw std::invalid_argument("BG2 cannot preserve a node transform; save a scene document");
    }
    return exportDrawableModelBg2(path, engine, node);
}

catch (const std::exception& error)
{
    std::fprintf(stderr, "Resource export failed: %s\n", error.what());
    return false;
}

catch (...)
{
    std::fprintf(stderr, "Resource export failed: unknown error\n");
    return false;
}


bool exportDrawableModelBg2(const std::filesystem::path& path, draw::Engine& engine, scene::Node* node) noexcept
try
{
    if (!node || !node->drawable() || !node->drawable()->requiresBg2Export())
    {
        throw std::invalid_argument("Model export requires an authored drawable owner");
    }
    if (!engine.device())
    {
        throw std::invalid_argument("Resource export requires an initialized draw Engine");
    }
    engine.device()->waitIdle();
    Publication publication(path);
    auto model = snapshot(*node, engine, publication);
    const auto main = publication.directory / publication.destination.filename();
    storeMeshBg2(main, &model);
    publication.commit(main);
    return true;
}

catch (const std::exception& error)
{
    std::fprintf(stderr, "Model export failed: %s\n", error.what());
    return false;
}
catch (...)
{
    std::fprintf(stderr, "Model export failed: unknown error\n");
    return false;
}

}
