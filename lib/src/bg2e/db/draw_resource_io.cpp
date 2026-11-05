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
#include <bg2e/scene/TextComponent.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/utils/utils.hpp>
#include "../base/TextDiagnostics.hpp"

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
// Unique resource generations avoid overwriting files referenced by the old
// document. The main file is the only replacement and is published last.
class Publication {
  public:
    explicit Publication(const std::filesystem::path& target)
        : destination(std::filesystem::absolute(target)), prefix(utils::uniqueId())
    {
        if (target.empty() || destination.filename().empty() ||
            destination.filename() == "." || destination.filename() == "..")
        {
            throw std::invalid_argument("Export requires an output filename");
        }
        // Save dialogs may introduce a scene folder that does not exist yet.
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
            for (const auto& path : published)
            {
                std::filesystem::remove(path, error);
            }
        }
        std::filesystem::remove_all(directory, error);
    }

    std::filesystem::path resource(const std::string& extension)
    {
        return directory / (prefix + "-" + std::to_string(sequence++) + extension);
    }

    std::filesystem::path imageResource(const std::filesystem::path& source)
    {
        const auto target = directory / source.filename();
        if (source.filename() == destination.filename())
        {
            throw std::invalid_argument("Image filename conflicts with the scene document");
        }
        // Filename identity is the current resource-sharing contract.
        if (!std::filesystem::exists(target))
        {
            const auto existing = destination.parent_path() / source.filename();
            std::filesystem::copy_file(std::filesystem::exists(existing) ? existing : source, target);
        }
        reusedNames.insert(source.filename());
        return target;
    }

    // Unlike legacy model-image filename sharing, embedded resources use full
    // normalized source identity. Equal basenames never alias unrelated files.
    std::filesystem::path externalResource(const std::filesystem::path& source)
    {
        const auto key = std::filesystem::weakly_canonical(source);
        if (const auto found = external.find(key); found != external.end())
        {
            return found->second;
        }
        auto target = resource(source.extension().string());
        std::filesystem::copy_file(source, target);
        external.emplace(key, target);
        return target;
    }

    void commit(const std::filesystem::path& main)
    {
        // Collect before modifying the directory iterator's directory.
        std::vector<std::filesystem::path> resources;
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.path() != main)
            {
                resources.push_back(entry.path());
            }
        }
        published.reserve(resources.size());
        for (const auto& source : resources)
        {
            auto target = destination.parent_path() / source.filename();
            if (std::filesystem::exists(target) && reusedNames.contains(source.filename()))
            {
                continue; // Reuse already consolidated images by filename.
            }
            published.push_back(target); // Reserve rollback bookkeeping first.
            std::error_code error;
            std::filesystem::create_hard_link(source, target, error);
            if (error)
            {
                // External/removable filesystems may not support hard links.
                // Never replace a resource owned by another saved document.
                published.pop_back();
                if (error == std::errc::file_exists)
                {
                    throw std::filesystem::filesystem_error("Export resource already exists", source, target, error);
                }
                // Reserve exclusive ownership so rollback also removes a
                // partial output if copying fails.
                published.push_back(target);
#ifdef _WIN32
                auto* output = _wfopen(target.c_str(), L"wbx");
#else
                auto* output = std::fopen(target.c_str(), "wbx");
#endif
                if (!output)
                {
                    published.pop_back();
                    throw std::runtime_error("Could not reserve export resource: " + target.string());
                }
                if (std::fclose(output) != 0)
                {
                    throw std::runtime_error("Could not close export resource: " + target.string());
                }
                std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing);
            }
        }
#ifdef _WIN32
        if (!MoveFileExW(main.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Publish document");
        }
#else
        std::filesystem::rename(main, destination);
#endif
        committed = true;
    }

    std::filesystem::path destination;
    std::filesystem::path directory;

  private:
    std::string prefix;
    size_t sequence = 0;
    bool committed = false;
    std::vector<std::filesystem::path> published;
    std::set<std::filesystem::path> reusedNames;
    std::map<std::filesystem::path, std::filesystem::path> external;
};

struct ReferenceRestore {
    std::vector<std::pair<scene::DrawableComponent*, std::filesystem::path>> paths;
    std::vector<std::pair<base::MaterialAttributes*, base::MaterialAttributes>> materials;

    ~ReferenceRestore()
    {
        for (auto& [material, attributes] : materials)
        {
            using std::swap;
            swap(*material, attributes);
        }
        for (auto& [component, path] : paths)
        {
            component->swapResourcePath(path);
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

Bg2Mesh snapshot(scene::Node& node, draw::Engine& engine, Publication& publication)
{
    auto* component = node.getComponent<scene::DrawableComponent>();
    auto drawable = component ? component->drawable() : nullptr;
    if (!drawable || !drawable->isLoaded() || drawable->engine() != &engine)
    {
        throw std::invalid_argument("Complete export requires a realized drawable from the supplied draw Engine");
    }
    Bg2Mesh model;
    model.mesh = std::make_shared<geo::MeshPNUUT>();
    for (uint32_t sub = 0; sub < drawable->submeshesCount(); ++sub)
    {
        auto material = drawable->material(sub);
        if (!material.isUpdated())
        {
            throw std::logic_error("Resolve pending or failed texture edits before exporting the document");
        }
        model.submeshNames.push_back(drawable->submeshName(sub));
        material.setGroupName(drawable->submeshGroupName(sub));
        material.setVisible(drawable->submeshVisibility(sub));
        auto live = drawable->drawMaterial(sub);
        for (uint32_t slot = 0; slot < 6; ++slot)
        {
            const auto authored = (material.*Getters[slot])();
            if (!authored || authored->ignoreSerialization())
            {
                continue;
            }
            auto settings = draw::cloneTextureSettings(*authored);
            std::filesystem::path path;
            if (authored->type() == base::Texture::TypeFilesystem && !authored->imageFilePath().empty())
            {
                path = publication.imageResource(authored->imageFilePath());
            }
            else
            {
                auto texture = live->texture(static_cast<draw::TextureSlot>(slot));
                if (!texture || texture->engine() != &engine || !texture->image())
                {
                    throw std::invalid_argument("Export texture belongs to another context or has no image");
                }
                path = publication.resource(texture->image()->pixelFormat() == gpu::PixelFormat::R32G32B32A32_SFLOAT
                    ? ".hdr" : ".png");
                saveImage(path, *texture->image());
            }
            settings->setImageFilePath(path.string());
            (material.*Setters[slot])(settings);
        }
        model.materials.push_back(std::move(material));
        const auto transform = drawable->submeshTransform(sub);
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                if (!std::isfinite(transform[column][row]))
                {
                    throw std::invalid_argument("Cannot export a non-finite drawable transform");
                }
            }
        }
        const auto determinant = glm::determinant(glm::mat3(transform));
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f)
        {
            throw std::invalid_argument("Cannot export a singular drawable transform");
        }
        const auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
        const auto range = drawable->submeshData(sub);
        const uint32_t first = static_cast<uint32_t>(model.mesh->indices.size());
        for (uint32_t i = 0; i < range.indexCount; ++i)
        {
            auto vertex = drawable->mesh()->vertices.at(drawable->mesh()->indices.at(range.firstIndex + i));
            vertex.position = glm::vec3(transform * glm::vec4(vertex.position, 1));
            vertex.normal = normalMatrix * vertex.normal;
            if (glm::length(vertex.normal) > 1e-8f)
            {
                vertex.normal = glm::normalize(vertex.normal);
            }
            vertex.tangent = glm::vec3(0); // BG2 reload regenerates tangents from geometry and UVs.
            model.mesh->indices.push_back(static_cast<uint32_t>(model.mesh->vertices.size()));
            model.mesh->vertices.push_back(vertex);
        }
        model.mesh->submeshes.push_back({first, range.indexCount});
    }
    if (auto joint = node.getComponent<scene::InputChainJointComponent>())
    {
        model.inputJoint = joint->joint();
    }
    if (auto joint = node.getComponent<scene::OutputChainJointComponent>())
    {
        model.outputJoint = joint->joint();
    }
    return model;
}

bool stageGraph(scene::Node& node, draw::Engine& engine, Publication& publication, ReferenceRestore& restore)
{
    if (auto component = node.getComponent<scene::DrawableComponent>(); component && !component->ignoreSerialization())
    {
        if (component->requiresBg2Export())
        {
            auto model = snapshot(node, engine, publication);
            auto path = publication.resource(".bg2");
            storeMeshBg2(path, &model);
            restore.paths.emplace_back(component, std::move(path));
            component->swapResourcePath(restore.paths.back().second);
        }
        else if (auto* text = dynamic_cast<scene::TextComponent*>(component))
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
                        base::detail::textWarning("Cannot export an embedded text material image; scene save refused.");
                        return false;
                    }
                    path = publication.resource(".png");
                    saveImage(path, pixels->data(), pixels->width(), pixels->height(), pixels->channels());
                }
                settings->setImageFilePath(path.string());
                (attributes.*Setters[slot])(settings);
            }
            // Install only at this stopped IO boundary. RAII restores the exact
            // authored material (including revisions) on success and failure.
            restore.materials.emplace_back(&text->material(), std::move(attributes));
            using std::swap;
            swap(text->material(), restore.materials.back().second);
        }
    }
    for (const auto& child : node.children())
    {
        if (!stageGraph(*child, engine, publication, restore))
        {
            return false;
        }
    }
    return true;
}

// External font/notice/environment files keep their original bytes/extensions.
// Custom components still require their own explicit export contract.
bool stageExternalReferences(const std::shared_ptr<json::JsonNode>& value, Publication& publication)
{
    if (!value)
    {
        return true;
    }
    if (value->isObject())
    {
        auto& object = value->objectValue();
        auto found = object.find("equirectangularTexture");
        const auto type = object.find("type");
        if (type != object.end() && type->second->isString() && type->second->stringValue() == "Environment" &&
            found != object.end() && found->second->isString() && !found->second->stringValue().empty())
        {
            auto source = publication.directory / found->second->stringValue();
            const auto path = publication.resource(source.extension().string());
            std::filesystem::copy_file(source, path);
            found->second = json::JSON(path.filename().string());
        }
        if (type != object.end() && type->second->isString() && type->second->stringValue() == "TextFonts")
        {
            auto fonts = object.find("fonts");
            if (fonts != object.end() && fonts->second && fonts->second->isList())
            {
                for (const auto& font : fonts->second->listValue())
                {
                    json::ObjectReader reader(font);
                    auto path = reader.getString("path");
                    if (!path || path->empty())
                    {
                        base::detail::textWarning("Cannot package a font without its authored path; save refused.");
                        return false;
                    }
                    const auto source = publication.directory / *path;
                    std::set<std::filesystem::path> notices;
                    if (auto supplied = reader.getArray("notices"))
                    {
                        for (size_t i = 0; i < supplied->size(); ++i)
                        {
                            auto notice = supplied->getString(i);
                            if (!notice || notice->empty())
                            {
                                base::detail::textWarning("Invalid supplied font notice path; save refused.");
                                return false;
                            }
                            notices.insert(publication.directory / *notice);
                        }
                    }
                    // Bundled tutorial font notices are adjacent. Recognize
                    // common supplied licenses without claiming legal rights.
                    if (notices.empty())
                    {
                        for (const char* name : {"OFL.txt", "LICENSE", "LICENSE.txt", "LICENSE.md",
                            "COPYING", "NOTICE", "NOTICE.txt", "README.md"})
                        {
                            const auto notice = source.parent_path() / name;
                            if (std::filesystem::is_regular_file(notice))
                            {
                                notices.insert(notice);
                            }
                        }
                    }
                    json::JsonList packaged;
                    if (notices.empty())
                    {
                        base::detail::textWarning("No supplied/adjacent font notices found for ", source.string(),
                            "; verify redistribution rights and provide required notices.");
                    }
                    for (const auto& notice : notices)
                    {
                        packaged.push_back(json::JSON(publication.externalResource(notice).filename().generic_string()));
                    }
                    const auto target = publication.externalResource(source);
                    font->objectValue()["path"] = json::JSON(target.filename().generic_string());
                    font->objectValue()["notices"] = json::JSON(packaged);
                }
            }
        }
        for (auto& [key, child] : object)
        {
            if (!stageExternalReferences(child, publication))
            {
                return false;
            }
        }
    }
    else if (value->isList())
    {
        for (auto& child : value->listValue())
        {
            if (!stageExternalReferences(child, publication))
            {
                return false;
            }
        }
    }
    return true;
}
}

bool saveSceneReferences(scene::Node* root, const std::filesystem::path& filePath,
    scene::SceneProgressCallback onProgress) noexcept
try
{
    if (!root || filePath.empty())
    {
        base::detail::textWarning("Scene reference save requires a root and path; save refused.");
        return false;
    }
    Publication publication(filePath);
    scene::SceneSaveProgress progress;
    progress.callback = onProgress;
    const auto count = [](auto&& self, const scene::Node* node) -> int
    {
        int result = 1;
        for (const auto& child : node->children())
        {
            result += self(self, child.get());
        }
        return result;
    };
    progress.total = count(count, root);
    auto representation = root->serializeReferences(publication.destination.parent_path(), onProgress ? &progress : nullptr);
    if (!representation)
    {
        base::detail::textWarning("Scene representation unavailable; destination unchanged.");
        return false;
    }
    using namespace json;
    auto document = JSON(JsonObject{
        {"fileType", JSON("bg2e::scene")},
        {"version", JSON(JsonObject{{"major", JSON(1)}, {"minor", JSON(0)}, {"rev", JSON(0)}})},
        {"scene", JSON(JsonList{representation})}});
    const auto main = publication.directory / publication.destination.filename();
    writeSceneDocument(document, main);
    publication.commit(main);
    return true;
}
catch (...)
{
    base::detail::textWarning("Scene reference serialization/write failed; destination unchanged.");
    return false;
}


bool saveScene(scene::Node* root, draw::Engine& engine, const std::filesystem::path& path,
    scene::SceneProgressCallback progress) noexcept
try
{
    if (!root || path.empty())
    {
        throw std::invalid_argument("Complete scene export requires a root and output path");
    }
    if (!engine.device())
    {
        throw std::invalid_argument("Resource export requires an initialized draw Engine");
    }
    engine.device()->waitIdle();
    Publication publication(path);
    ReferenceRestore restore;
    if (!stageGraph(*root, engine, publication, restore))
    {
        return false;
    }
    const auto main = publication.directory / publication.destination.filename();
    if (!saveSceneReferences(root, main, progress)) // Custom virtual serializers remain authoritative.
    {
        return false;
    }
    auto document = readSceneDocument(main);
    if (!stageExternalReferences(document, publication))
    {
        return false;
    }
    writeSceneDocument(document, main);
    publication.commit(main);
    return true;
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

bool storeDrawableBg2(const std::filesystem::path& path, draw::Engine& engine, scene::Node* node) noexcept
try
{
    if (!node || !node->children().empty() || !node->enabled() || node->steady())
    {
        throw std::invalid_argument("BG2 export requires a single model node; use a scene document for a hierarchy");
    }
    if (auto* component = node->drawable(); component && !component->requiresBg2Export())
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
    std::fprintf(stderr, "Resource export failed: %s\n", error.what());
    return false;
}

catch (...)
{
    std::fprintf(stderr, "Resource export failed: unknown error\n");
    return false;
}

}
