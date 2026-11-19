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

#include <bg2e/db/SceneResourceManager.hpp>
#include <bg2e/db/scene.hpp>
#include <bg2e/db/scene_path.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <cgltf.h>
#include <cctype>
#include <bg2e/json/NodeReader.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <stdexcept>

namespace bg2e::db {
namespace {
namespace fs = std::filesystem;

fs::path resolved(const fs::path& base, const fs::path& source)
{
    return fs::weakly_canonical(fs::absolute(base / source));
}

bool internal(const fs::path& root, const fs::path& path)
{
    const auto relative = path.lexically_relative(root);
    return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
}

SceneResourceFile inspect(const fs::path& path)
{
    SceneResourceFile file;
    file.path = path;
    if (!fs::exists(path))
    {
        return file;
    }
    if (!fs::is_regular_file(path))
    {
        throw std::runtime_error("Not a regular resource file: " + path.string());
    }
    file.bytes = fs::file_size(path);
    file.modified = fs::last_write_time(path);
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("Cannot inspect resource: " + path.string());
    }
    uint64_t hash = 14695981039346656037ull;
    std::array<char, 65536> buffer;
    while (input.read(buffer.data(), buffer.size()) || input.gcount())
    {
        for (std::streamsize i = 0; i < input.gcount(); ++i)
        {
            hash = (hash ^ static_cast<unsigned char>(buffer[i])) * 1099511628211ull;
        }
    }
    if (!input.eof())
    {
        throw std::runtime_error("Cannot read resource: " + path.string());
    }
    file.fingerprint = hash;
    return file;
}

SceneResourceFile inspectWithWarnings(const fs::path& path, std::vector<std::string>& warnings)
{
    try
    {
        return inspect(path);
    }
    catch (const std::exception& error)
    {
        warnings.push_back(error.what());
        SceneResourceFile file;
        file.path = path;
        std::error_code ignored;
        const auto bytes = fs::file_size(path, ignored);
        if (!ignored)
        {
            file.bytes = bytes;
        }
        const auto modified = fs::last_write_time(path, ignored);
        if (!ignored)
        {
            file.modified = modified;
        }
        return file;
    }
}

bool sameAnalysis(const SceneResourceAnalysis& a, const SceneResourceAnalysis& b)
{
    return a.scenePath == b.scenePath && a.protection == b.protection && a.snapshot == b.snapshot && a.resources == b.resources &&
        a.externalResources == b.externalResources && a.unusedFiles == b.unusedFiles &&
        a.symbolicLinks == b.symbolicLinks && a.warnings == b.warnings;
}

void collect(std::shared_ptr<json::JsonNode> node, const fs::path& base, SceneFileReferences& result)
{
    json::ObjectReader reader(node);
    if (!reader.isValid())
    {
        result.warnings.push_back("Invalid scene node; dependencies may be incomplete.");
        return;
    }
    const auto add = [&](const json::ObjectReader& object, const char* key)
    {
        if (auto value = object.getString(key))
        {
            if (!value->empty())
            {
                result.files.push_back(fs::absolute(base / *value).lexically_normal());
            }
        }
        else if (object.getNode(key) && !object.isNull(key))
        {
            result.warnings.push_back(std::string("Invalid resource reference: ") + key);
        }
    };
    for (const auto key : {"components", "children"})
    {
        if (reader.getNode(key) && !reader.isArray(key))
        {
            result.warnings.push_back(std::string("Invalid scene node field: ") + key);
        }
    }
    if (auto components = reader.getArray("components"))
    {
        for (size_t i = 0; i < components->size(); ++i)
        {
            auto component = components->getObject(i);
            if (!component)
            {
                result.warnings.push_back("Invalid component; dependencies may be incomplete.");
                continue;
            }
            const auto type = component->getString("type").value_or("");
            if (type == "Drawable" || type == "DrawableInstance")
            {
                fs::path path;
                if (auto source = component->getString("path"); source && !source->empty())
                {
                    path = base / *source;
                }
                else if (auto name = component->getString("name"); name && !name->empty())
                {
                    path = base / *name;
                    path.replace_extension(".bg2");
                }
                if (!path.empty())
                {
                    path = fs::absolute(path).lexically_normal();
                    result.files.push_back(path);
                    result.models.push_back(path);
                }
                else
                {
                    result.warnings.push_back(type + " without a model reference.");
                }
                if (type == "DrawableInstance")
                {
                    if (auto submeshes = component->getArray("submeshes"))
                    {
                        for (size_t sub = 0; sub < submeshes->size(); ++sub)
                        {
                            const auto item = submeshes->getObject(sub);
                            const auto material = item ? item->getObject("material") : std::nullopt;
                            if (!item)
                            {
                                result.warnings.push_back("Invalid instance submesh; dependencies may be incomplete.");
                                continue;
                            }
                            if (!material)
                            {
                                if (!item->isUndefined("material"))
                                {
                                    result.warnings.push_back("Invalid instance material; dependencies may be incomplete.");
                                }
                                continue;
                            }
                            for (const auto* key : {"albedoTexture", "metalnessTexture", "roughnessTexture",
                                "normalTexture", "ambientOcclussion", "lightEmissionTexture"})
                            {
                                add(*material, key);
                            }
                        }
                    }
                    else if (!component->isUndefined("submeshes"))
                    {
                        result.warnings.push_back("Invalid instance submeshes; dependencies may be incomplete.");
                    }
                }
            }
            else if (type == "Environment")
            {
                add(*component, component->getString("equirectangularTexture") ?
                    "equirectangularTexture" : "environmentImage");
            }
            else if (type == "TextFonts")
            {
                if (auto fonts = component->getArray("fonts"))
                {
                    for (size_t j = 0; j < fonts->size(); ++j)
                    {
                        auto font = fonts->getObject(j);
                        if (!font)
                        {
                            result.warnings.push_back("Invalid font declaration.");
                            continue;
                        }
                        add(*font, "path");
                        if (auto notices = font->getArray("notices"))
                        {
                            for (size_t k = 0; k < notices->size(); ++k)
                            {
                                if (auto notice = notices->getString(k); notice && !notice->empty())
                                {
                                    result.files.push_back(fs::absolute(base / *notice).lexically_normal());
                                }
                                else
                                {
                                    result.warnings.push_back("Invalid font notice reference.");
                                }
                            }
                        }
                    }
                }
            }
            else if (type == "Text")
            {
                if (auto material = component->getObject("material"))
                {
                    for (const auto key : {"albedoTexture", "metalnessTexture", "roughnessTexture",
                        "normalTexture", "ambientOcclussion", "lightEmissionTexture"})
                    {
                        // Numeric material values are not filesystem references.
                        if (material->isString(key))
                        {
                            add(*material, key);
                        }
                    }
                }
            }
            else
            {
                static const std::set<std::string> noFiles{"Transform", "Camera", "LightSource", "Chain",
                    "InputChainJoint", "OutputChainJoint", "PolarTransformController", "FixedScaleTransform",
                    "OrbitCameraController"};
                if (!noFiles.contains(type))
                {
                    result.warnings.push_back("Unsupported component: " + type + "; dependencies may be incomplete.");
                }
            }
        }
    }
    if (auto children = reader.getArray("children"))
    {
        for (size_t i = 0; i < children->size(); ++i)
        {
            collect(children->getNode(i), base, result);
        }
    }
}
}

SceneFileReferences sceneFileReferences(std::shared_ptr<json::JsonNode> node, const fs::path& basePath)
{
    SceneFileReferences result;
    collect(std::move(node), basePath, result);
    return result;
}

SceneResourceAnalysis SceneResourceManager::analyze(const fs::path& selectedPath)
{
    return analyze(selectedPath, {});
}


SceneResourceAnalysis SceneResourceManager::analyze(const fs::path& selectedPath, const SceneResourceProtection& protection)
{
    SceneResourceAnalysis result;
    result.protection = protection;
    result.scenePath = fs::canonical(resolveSceneLoadPath(selectedPath));
    const auto root = result.scenePath.parent_path();
    const auto document = readSceneDocument(result.scenePath);
    const auto nodes = json::ObjectReader(document).getArray("scene");
    std::set<fs::path> resources{result.scenePath};
    std::set<fs::path> models;
    for (size_t i = 0; i < nodes->size(); ++i)
    {
        auto references = sceneFileReferences(nodes->getNode(i), root);
        result.warnings.insert(result.warnings.end(), references.warnings.begin(), references.warnings.end());
        for (const auto& path : references.files)
        {
            resources.insert(resolved(root, path));
        }
        for (const auto& path : references.models)
        {
            models.insert(resolved(root, path));
        }
    }
    for (const auto& path : protection.roots.files)
    {
        resources.insert(resolved(root, path));
    }
    for (const auto& path : protection.roots.models)
    {
        models.insert(resolved(root, path));
    }
    result.warnings.insert(result.warnings.end(), protection.roots.warnings.begin(), protection.roots.warnings.end());
    std::set<fs::path> visited;
    while (!models.empty())
    {
        const auto model = *models.begin();
        models.erase(models.begin());
        resources.insert(model);
        if (!visited.insert(model).second)
        {
            continue;
        }
        auto extension = model.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char value)
        {
            return static_cast<char>(std::tolower(value));
        });
        try
        {
            const auto add = [&](const fs::path& path)
            {
                resources.insert(resolved(model.parent_path(), path));
            };
            if (extension == ".bg2" || extension == ".vwglb")
            {
                for (const auto& texture : meshTexturePaths(model))
                {
                    add(texture);
                }
            }
            else if (extension == ".obj")
            {
                const auto parsed = readObj(model);
                for (const auto& source : parsed->sourcePaths)
                {
                    add(source);
                }
                for (const auto& material : parsed->materials)
                {
                    for (const auto& texture : {material.albedoTexture(), material.metalnessTexture(), material.roughnessTexture(),
                        material.normalTexture(), material.aoTexture(), material.lightEmissionTexture()})
                    {
                        if (texture && texture->type() == base::Texture::TypeFilesystem && !texture->imageFilePath().empty())
                        {
                            add(texture->imageFilePath());
                        }
                    }
                }
            }
            else if (extension == ".gltf" || extension == ".glb")
            {
                cgltf_options options{};
                cgltf_data* raw = nullptr;
                const auto status = cgltf_parse_file(&options, model.string().c_str(), &raw);
                const std::unique_ptr<cgltf_data, decltype(&cgltf_free)> parsed(raw, &cgltf_free);
                if (status != cgltf_result_success)
                {
                    throw std::runtime_error("Cannot inspect glTF dependencies: " + model.string());
                }
                const auto uri = [&](const char* source)
                {
                    if (!source || std::string(source).starts_with("data:"))
                    {
                        return;
                    }
                    std::string path(source);
                    if (path.find(':') != std::string::npos)
                    {
                        result.warnings.push_back("Unsupported glTF dependency URI: " + path);
                        return;
                    }
                    path.resize(cgltf_decode_uri(path.data()));
                    add(path);
                };
                for (cgltf_size i = 0; i < parsed->buffers_count; ++i)
                {
                    uri(parsed->buffers[i].uri);
                }
                for (cgltf_size i = 0; i < parsed->images_count; ++i)
                {
                    uri(parsed->images[i].uri);
                }
            }
            else if (extension == ".json")
            {
                const auto nested = json::ObjectReader(readSceneDocument(model)).getArray("scene");
                for (size_t i = 0; i < nested->size(); ++i)
                {
                    const auto references = sceneFileReferences(nested->getNode(i), model.parent_path());
                    for (const auto& file : references.files)
                    {
                        add(file);
                    }
                    for (const auto& child : references.models)
                    {
                        models.insert(resolved(model.parent_path(), child));
                    }
                    result.warnings.insert(result.warnings.end(), references.warnings.begin(), references.warnings.end());
                }
            }
            else
            {
                result.warnings.push_back("Unsupported model dependencies: " + model.string());
            }
        }
        catch (const std::exception& error)
        {
            result.warnings.push_back(error.what());
        }
    }
    for (const auto& path : resources)
    {
        result.resources.push_back(path);
        if (!internal(root, path))
        {
            result.externalResources.push_back(path);
        }
        if (!fs::exists(path))
        {
            result.warnings.push_back("Missing resource: " + path.string());
        }
        result.snapshot.emplace(path, inspectWithWarnings(path, result.warnings));
    }
    // Never follow directory links or remove links themselves.
    for (const auto& entry : fs::recursive_directory_iterator(root))
    {
        if (entry.is_symlink())
        {
            result.symbolicLinks.push_back(entry.path());
            result.warnings.push_back("Symbolic link excluded from cleanup: " + entry.path().string());
            continue;
        }
        if (!entry.is_regular_file())
        {
            continue;
        }
        auto file = inspectWithWarnings(entry.path(), result.warnings);
        result.snapshot[file.path] = file;
        if (!resources.contains(resolved(root, file.path)))
        {
            result.unusedFiles.push_back(std::move(file));
        }
    }
    std::sort(result.unusedFiles.begin(), result.unusedFiles.end(), [](const auto& a, const auto& b)
    {
        return a.path < b.path;
    });
    std::sort(result.symbolicLinks.begin(), result.symbolicLinks.end());
    std::sort(result.warnings.begin(), result.warnings.end());
    result.warnings.erase(std::unique(result.warnings.begin(), result.warnings.end()), result.warnings.end());
    return result;
}

SceneCleanupPlan SceneResourceManager::createCleanupPlan(const SceneResourceAnalysis& analysis, bool ignoreWarnings)
{
    return createCleanupPlan(analysis, {}, ignoreWarnings);
}


SceneCleanupPlan SceneResourceManager::createCleanupPlan(const SceneResourceAnalysis& analysis,
    const SceneResourceProtection& protection, bool ignoreWarnings)
{
    if (!ignoreWarnings && !analysis.warnings.empty())
    {
        throw std::runtime_error("Review and explicitly acknowledge dependency warnings before cleanup.");
    }
    if (!sameAnalysis(analyze(analysis.scenePath, protection), analysis))
    {
        throw std::runtime_error("Scene files changed. Analyze resources again before cleanup.");
    }
    SceneCleanupPlan plan;
    plan._analysis = analysis;
    plan._ignoreWarnings = ignoreWarnings;
    return plan;
}


SceneCleanupPlan SceneResourceManager::rebindCleanupPlan(const SceneCleanupPlan& plan, const SceneResourceProtection& protection)
{
    const auto& previous = plan._analysis;
    for (const auto& [identity, generation] : previous.protection.cacheGenerations)
    {
        const auto current = protection.cacheGenerations.find(identity);
        if (current == protection.cacheGenerations.end() || current->second != generation)
        {
            throw std::runtime_error("Resident drawable cache changed. Analyze resources again before cleanup.");
        }
    }
    auto current = analyze(previous.scenePath, protection);
    const auto contains = [](const auto& values, const auto& value)
    {
        return std::find(values.begin(), values.end(), value) != values.end();
    };
    bool safe = current.snapshot == previous.snapshot && current.symbolicLinks == previous.symbolicLinks;
    for (const auto& path : previous.resources)
    {
        safe = safe && contains(current.resources, path);
    }
    for (const auto& warning : current.warnings)
    {
        safe = safe && contains(previous.warnings, warning);
    }
    for (const auto& file : current.unusedFiles)
    {
        safe = safe && contains(previous.unusedFiles, file);
    }
    if (!safe)
    {
        throw std::runtime_error("Scene resources changed during reload. Analyze resources again before cleanup.");
    }
    SceneCleanupPlan rebound;
    rebound._analysis = std::move(current);
    rebound._ignoreWarnings = plan._ignoreWarnings;
    return rebound;
}


SceneCleanupResult SceneResourceManager::executeCleanup(const SceneCleanupPlan& plan)
{
    return executeCleanup(plan, {});
}


SceneCleanupResult SceneResourceManager::executeCleanup(const SceneCleanupPlan& plan, const SceneResourceProtection& protection)
{
    const auto& previous = plan._analysis;
    const auto current = analyze(previous.scenePath, protection);
    if (!sameAnalysis(current, previous))
    {
        throw std::runtime_error("Scene files changed. Analyze resources again before cleanup.");
    }
    if (!plan._ignoreWarnings && !current.warnings.empty())
    {
        throw std::runtime_error("Dependency warnings have not been acknowledged.");
    }
    SceneCleanupResult result;
    const auto root = current.scenePath.parent_path();
    for (const auto& file : current.unusedFiles)
    {
        try
        {
            // Recheck the resolved destination immediately before each removal.
            std::vector<std::string> warnings;
            if (!internal(root, file.path) || fs::canonical(file.path) != file.path ||
                fs::is_symlink(fs::symlink_status(file.path)) ||
                inspectWithWarnings(file.path, warnings) != file)
            {
                throw std::runtime_error("Resource changed during cleanup: " + file.path.string());
            }
            if (!fs::remove(file.path))
            {
                throw std::runtime_error("Could not remove resource: " + file.path.string());
            }
            result.deleted.push_back(file.path);
        }
        catch (const std::exception& error)
        {
            result.errors.push_back(error.what());
        }
    }
    return result;
}

}
